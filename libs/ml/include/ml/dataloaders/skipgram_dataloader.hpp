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

#include "core/random/lcg.hpp"
#include "core/random/lfg.hpp"
#include "math/free_functions/standard_functions/abs.hpp"
#include "ml/dataloaders/text_loader.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <dirent.h>  // may be compatibility issues

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * additional params only relent for Skipgram models
 */
template <typename T>
struct SkipGramTextParams : TextParams<T>
{
public:
  SkipGramTextParams()
    : TextParams<T>(){};

  using SizeType              = typename T::SizeType;
  SizeType k_negative_samples = 0;  // # negative samples per positive training pair
};

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class SkipGramLoader : public TextLoader<T>
{
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

private:
  // training data parsing containers
  SizeType pos_size_ = 0;  // # positive training pairs
  SizeType neg_size_ = 0;  // # negative training pairs

  SkipGramTextParams<T> p_;

public:
  SkipGramLoader(std::string &data, SkipGramTextParams<T> p, SizeType seed = 123456789)
    : TextLoader<T>(data, p, seed)
    , p_(p)
  {

    // sanity checks on SkipGram parameters
    assert(this->word_count_ > (p_.window_size * 2));
    assert(p_.window_size > 0);
  }

private:
  /**
   * get a single training pair from a word index
   * @param idx
   * @return
   */
  virtual std::vector<SizeType> GetData(SizeType idx) override
  {
    if (SelectValence())
    {
      return GeneratePositive(idx);
    }
    {
      return GenerateNegative(idx);
    }
  }

  /**
   * randomly select whether to return a positive or negative example
   */
  bool SelectValence()
  {
    double cur_val = this->lfg_.AsDouble();
    double positive_threshold;
    if (p_.k_negative_samples > 0)
    {
      positive_threshold = 1.0 / p_.k_negative_samples;
    }
    else
    {
      positive_threshold = 1.0;
    }
    std::vector<SizeType> ret{};

    if (cur_val < positive_threshold)
    {
      return true;
    }
    return false;
  }

  /**
   * given the index of the input word, return the positive training pair
   * @param idx
   * @return
   */
  std::vector<SizeType> GeneratePositive(SizeType idx)
  {
    std::vector<SizeType> ret{};

    // first index is the input word
    ret.emplace_back(idx);

    // second index is a context word
    ret.emplace_back(SelectContextPosition(idx));

    // finally the label
    ret.emplace_back(SizeType(1));

    return ret;
  }

  /**
   * given the index of the input word, return a negative training pair
   * @param idx
   * @return
   */
  std::vector<SizeType> GenerateNegative(SizeType idx)
  {
    std::vector<SizeType> ret{};

    // first index is the input word
    ret.emplace_back(idx);

    // second index is a non-context word
    ret.emplace_back(SelectNegativeContextWord(idx));

    // finally the label
    ret.emplace_back(SizeType(0));

    return ret;
  }

  /**
   * given a word idx randomly select a negative non-context word
   * @param idx
   * @return
   */
  SizeType SelectNegativeContextWord(SizeType idx)
  {
    std::vector<SizeType> sentence     = this->GetSentenceFromWordIdx(idx);
    SizeType              sentence_len = sentence.size();
    SizeType              word_offset  = this->GetWordOffsetFromWordIdx(idx);

    // randomly select a negative context word
    bool     ongoing = true;
    SizeType negative_context_idx;
    while (ongoing)
    {
      // randomly select a word from the unigram_table
      SizeType ran_val     = SizeType(this->lcg_());
      SizeType max_val     = this->unigram_table_.size();
      SizeType idx         = ran_val % max_val;
      negative_context_idx = this->unigram_table_.at(idx);
      assert(negative_context_idx > 0);
      assert(negative_context_idx < this->vocab_.size());
      ongoing = false;

      // check if selected negative word actually within context window
      for (SizeType j = 0; j < (2 * p_.window_size) + 1; j++)
      {
        if (WindowPositionCheck(word_offset, j, sentence_len))
        {
          if (this->data_[this->word_idx_sentence_idx[idx]][j] == negative_context_idx)
          {
            ongoing = true;
          }
        }
      }
    }

    return negative_context_idx;
  }

  /**
   * select a context index position
   * @param idx
   * @return
   */
  SizeType SelectContextPosition(SizeType idx)
  {

    std::vector<SizeType> sentence     = this->GetSentenceFromWordIdx(idx);
    SizeType              sentence_len = sentence.size();
    SizeType              word_offset  = this->GetWordOffsetFromWordIdx(idx);

    // check all potential context positions
    double                current_probability;
    int                   normalised_context_position;
    std::vector<SizeType> unigram_selection;

    for (SizeType j = 0; j < (1 + (2 * p_.window_size)); ++j)
    {
      normalised_context_position = int(j) - int(p_.window_size);
      int abs_dist_to_target      = fetch::math::Abs(normalised_context_position);
      if (WindowPositionCheck(word_offset, j, sentence_len))
      {
        current_probability = 1.0 / abs_dist_to_target;

        for (int k = 0; k < int(current_probability * double(p_.unigram_precision)); ++k)
        {
          unigram_selection.push_back(j);
        }
      }
    }

    SizeType context_offset = unigram_selection.at(this->lcg_() % unigram_selection.size());
    SizeType context_idx    = idx + context_offset - p_.window_size;

    return context_idx;
  }

  /**
   * checks if a context position is valid for the sentence
   * @param target_pos current target position in sentence
   * @param context_pos current context position in sentence
   * @param sentence_len total sentence length
   * @return
   */
  bool WindowPositionCheck(SizeType target_pos, SizeType context_pos, SizeType sentence_len) const
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
};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
