#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "math/standard_functions/abs.hpp"
#include "ml/dataloaders/word2vec_loaders/basic_textloader.hpp"

#include <chrono>

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * additional params only relevant for Skipgram models
 */
template <typename T>
struct SkipGramTextParams : TextParams<T>
{
public:
  SkipGramTextParams()
    : TextParams<T>(false){};

  using SizeType              = typename TextParams<T>::SizeType;
  SizeType k_negative_samples = 0;  // # negative samples per positive training pair

  bool unigram_table = true;  // build a unigram table

  // unigram params
  SizeType unigram_table_size = 10000000;  // size of unigram table for negative sampling
  double   unigram_power      = 0.75;      // adjusted unigram distribution
  SizeType unigram_precision  = 10;
};

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class SkipGramLoader : public BasicTextLoader<T>
{
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

private:
  // training data parsing containers
  SkipGramTextParams<T> p_;

  // the unigram table container
  std::vector<SizeType> unigram_table_;

  double positive_threshold_ = 0.0;

  // current label for sampling
  SizeType cur_label_ = 0;

public:
  SkipGramLoader(SkipGramTextParams<T> p, SizeType seed = 123456789);

  virtual bool AddData(std::string const &training_data) override;

private:
  std::vector<SizeType> unigram_selection_;
  SizeType              max_window_size_;

  virtual void                                      GetData(SizeType idx, ArrayType &ret) override;
  virtual SizeType                                  GetLabel(SizeType idx) override;
  std::vector<typename SkipGramLoader<T>::SizeType> lookup_idxs_{{0, 0, 0}};

  void     BuildUnigramTable();
  bool     SelectValence();
  void     SelectNegativeContextWord(SizeType idx, SizeType &ret);
  SizeType SelectContextPosition(SizeType idx);
  bool WindowPositionCheck(SizeType target_pos, SizeType context_pos, SizeType sentence_len) const;
  void PreCalcDynamicWindow();
};
template <typename T>
SkipGramLoader<T>::SkipGramLoader(SkipGramTextParams<T> p, SizeType seed)
  : BasicTextLoader<T>(p, seed)
  , p_(p)
{
  assert(p_.window_size > 0);

  // sanity checks on SkipGram parameters
  assert(p_.window_size > 0);

  // set probabilities for sampling positive and negative training pairs
  if (p_.k_negative_samples > 0)
  {
    positive_threshold_ = 1.0 / (double(p_.k_negative_samples) + 1.0);
  }
  else
  {
    positive_threshold_ = 1.0;
  }

  max_window_size_ = (1 + (2 * p_.window_size));

  // pre-calculate dynamic window position probabilities
  PreCalcDynamicWindow();
}

/**
 * get a single training pair from a word index
 * @param idx
 * @return
 */
template <typename T>
void SkipGramLoader<T>::GetData(SizeType idx, T &data_buffer)
{

  lookup_idxs_[0] = idx;

  if (SelectValence())
  {
    // positive example
    lookup_idxs_[1] = SelectContextPosition(idx);
    lookup_idxs_[2] = 1;
  }
  else
  {
    // negative example
    SelectNegativeContextWord(idx, lookup_idxs_[1]);
    lookup_idxs_[2] = 0;
  }

  SizeType buffer_count = 0;
  for (SizeType j = 0; j < p_.n_data_buffers; ++j)
  {
    SizeType sentence_idx = this->word_idx_sentence_idx.at(lookup_idxs_.at(buffer_count));
    SizeType word_idx     = this->GetWordOffsetFromWordIdx(lookup_idxs_.at(buffer_count));
    data_buffer.At(j)     = DataType(this->data_.at(sentence_idx).at(word_idx));
    ++buffer_count;
  }

  cur_label_ = lookup_idxs_.at(p_.n_data_buffers);
}

/**
 * get a single training pair from a word index
 * @param idx
 * @return
 */
template <typename T>
typename SkipGramLoader<T>::SizeType SkipGramLoader<T>::GetLabel(SizeType /*idx*/)
{
  return cur_label_;
}

/**
 * randomly select whether to return a positive or negative example
 */
template <typename T>
bool SkipGramLoader<T>::SelectValence()
{

  double cur_val = this->lfg_.AsDouble();
  return (cur_val <= positive_threshold_);
}

/**
 * given a word idx randomly select a negative non-context word
 * @param idx
 * @return
 */
template <typename T>
void SkipGramLoader<T>::SelectNegativeContextWord(SizeType idx, SizeType &ret)
{
  SizeType sentence_len = this->data_.at(this->word_idx_sentence_idx.at(idx)).size();
  SizeType word_offset  = this->GetWordOffsetFromWordIdx(idx);

  // randomly select a negative context word
  bool ongoing = true;
  while (ongoing)
  {
    // randomly select a word from the unigram_table
    SizeType ran_val = SizeType(this->lcg_());
    SizeType max_val = this->unigram_table_.size();
    SizeType idx     = ran_val % max_val;
    ret              = this->unigram_table_.at(idx);
    assert(ret < this->vocab_.size());
    ongoing = false;

    // check if selected negative word actually within context window
    for (SizeType j = 0; j < (2 * p_.window_size) + 1; j++)
    {
      if (WindowPositionCheck(word_offset, j, sentence_len))
      {
        if (this->data_[this->word_idx_sentence_idx[idx]][j] == ret)
        {
          ongoing = true;
        }
      }
    }
  }
}

/**
 * select a context index position
 * @param idx
 * @return
 */
template <typename T>
typename SkipGramLoader<T>::SizeType SkipGramLoader<T>::SelectContextPosition(SizeType idx)
{

  SizeType sentence_len = this->data_.at(this->word_idx_sentence_idx.at(idx)).size();
  SizeType word_offset  = this->GetWordOffsetFromWordIdx(idx);

  bool not_found = true;
  SizeType context_offset = unigram_selection_.at(this->lcg_() % unigram_selection_.size());
  ASSERT(context_offset < max_window_size_);
  while (not_found)
  {
    if (WindowPositionCheck(word_offset, context_offset, sentence_len))
    {
      not_found = false;
    }
    else
    {
      context_offset = unigram_selection_.at(this->lcg_() % unigram_selection_.size());
    }
  }

  SizeType context_idx = idx + context_offset - p_.window_size;

  return context_idx;
}

/**
 * checks if a context position is valid for the sentence
 * @param target_pos current target position in sentence
 * @param context_pos current context position in sentence
 * @param sentence_len total sentence length
 * @return
 */
template <typename T>
bool SkipGramLoader<T>::WindowPositionCheck(SizeType target_pos, SizeType context_pos,
                                            SizeType sentence_len) const
{
  int normalised_context_pos = int(context_pos) - int(p_.window_size);
  if (normalised_context_pos == 0)
  {
    // context position is on top of target position
    return false;
  }
  else if (int(target_pos) + normalised_context_pos < 0)
  {
    // context position is before start of sentence
    return false;
  }
  else if (int(target_pos) + normalised_context_pos >= int(sentence_len))
  {
    // context position is after end of sentence
    return false;
  }
  else
  {
    return true;
  }
}

/**
 * called once by the constructor to pre-calculate the probabilities
 * for the dynamic context window selection.
 * However, this assumes every context window position is valid, which
 * is not true close to the start of end of the sentence, so this has
 * to be checked at runtime
 * @tparam T
 */
template <typename T>
void SkipGramLoader<T>::PreCalcDynamicWindow()
{
  // check all potential context positions
  double current_probability;
  int    normalised_context_position;

  for (int j = 0; j < int(max_window_size_); ++j)
  {
    normalised_context_position = j - int(p_.window_size);
    int abs_dist_to_target      = fetch::math::Abs(normalised_context_position);
    current_probability         = 1.0 / abs_dist_to_target;

    for (int k = 0; k < int(current_probability * double(p_.unigram_precision)); ++k)
    {
      unigram_selection_.emplace_back(j);
    }
  }
}

/**
 * For skipgram we need to build the unigram table as well
 * @tparam T
 */
template <typename T>
bool SkipGramLoader<T>::AddData(std::string const &training_data)
{
  bool success = false;

  // ordinary pre-processing
  success = BasicTextLoader<T>::AddData(training_data);

  if (success)
  {
    BuildUnigramTable();
  }
  return success;
}

/**
 * builds the unigram table for negative sampling
 */
template <typename T>
void SkipGramLoader<T>::BuildUnigramTable()
{
  if (p_.unigram_table)
  {
    unigram_table_.resize(p_.unigram_table_size);

    // calculate adjusted word frequencies
    double sum_adj_vocab = 0.0;
    double cur_val;
    for (auto &e : this->vocab_frequencies)
    {
      cur_val = std::pow(e.second, p_.unigram_power);
      sum_adj_vocab += cur_val;
    }

    assert(sum_adj_vocab > 0);

    SizeType cur_idx = 0;
    SizeType n_rows  = 0;
    for (auto &e : this->vocab_frequencies)
    {
      assert(cur_idx < p_.unigram_table_size);

      double adjusted_word_probability = std::pow(e.second, p_.unigram_power) / sum_adj_vocab;

      // word frequency
      n_rows = SizeType(adjusted_word_probability * double(p_.unigram_table_size));

      for (SizeType k = 0; k < n_rows; ++k)
      {
        unigram_table_.at(cur_idx) = e.first;
        ++cur_idx;
      }
    }
    unigram_table_.resize(cur_idx - 1);
    p_.unigram_table_size = cur_idx - 1;
  }
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
