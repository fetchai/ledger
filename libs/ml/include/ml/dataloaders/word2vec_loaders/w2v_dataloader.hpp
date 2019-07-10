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
#include "math/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/word2vec_loaders/unigram_table.hpp"
#include "ml/dataloaders/word2vec_loaders/vocab.hpp"

#include <exception>
#include <map>
#include <random>
#include <string>
#include <utility>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename T>
class W2VLoader : public DataLoader<fetch::math::Tensor<T>, fetch::math::Tensor<T>>
{
public:
	// The intended T is the typename for the data input to the neural network, which should be a float or double or fix-point type.
  static constexpr T WindowContextUnused = -1;

  using LabelType = fetch::math::Tensor<T>;
  using DataType  = fetch::math::Tensor<T>;

  using SizeType   = fetch::math::SizeType;
  using VocabType  = Vocab;
  using ReturnType = std::pair<LabelType, std::vector<DataType>>;

  W2VLoader(SizeType window_size, SizeType negative_samples, bool mode);

  bool       IsDone() const override;
  void       Reset() override;
  void       RemoveInfrequent(SizeType min);
  void       InitUnigramTable();
  void       GetNext(ReturnType &t);
  ReturnType GetNext() override;

  bool BuildVocab(std::string const &s);
  void SaveVocab(std::string const &filename);
  void LoadVocab(std::string const &filename);

  /// accessors and helper functions ///
  SizeType         Size() const override;
  SizeType         vocab_size() const;
  VocabType const &vocab() const;
  std::string      WordFromIndex(SizeType index) const;
  SizeType         IndexFromWord(std::string const &word) const;
  SizeType         window_size();

private:
  SizeType                                   current_sentence_;
  SizeType                                   current_word_;
  SizeType                                   window_size_;
  SizeType                                   negative_samples_;
  VocabType                                  vocab_;
  std::vector<std::vector<SizeType>>         data_;
  fetch::random::LinearCongruentialGenerator rng_;
  UnigramTable                               unigram_table_;
  bool                                       mode_;

  fetch::math::Tensor<T> target_;  // reusable target tensor
  fetch::math::Tensor<T> label_;   // reusable label tensor

  std::vector<SizeType>    StringsToIndices(std::vector<std::string> const &strings);
  std::vector<std::string> PreprocessString(std::string const &s);
};

/**
 *
 * @tparam T
 * @param window_size the size of the context window (one side only)
 * @param negative_samples the number of total samples (all but one being negative)
 * @param mode
 */
template <typename T>
W2VLoader<T>::W2VLoader(SizeType window_size, SizeType negative_samples, bool mode)
  : DataLoader<LabelType, DataType>(false)  // no random mode specified
  , current_sentence_(0)
  , current_word_(0)
  , window_size_(window_size)
  , negative_samples_(negative_samples)
  , mode_(mode)
  , target_({window_size_ * 2, 1})
  , label_({negative_samples_, 1})
{}

/**
 * reports the total size of the outputs iterating through the dataloader
 * @tparam T
 * @return
 */
template <typename T>
math::SizeType W2VLoader<T>::Size() const
{
  SizeType size(0);
  for (auto const &s : data_)
  {
    if ((SizeType)s.size() > (2 * window_size_))
    {
      size += (SizeType)s.size() - (2 * window_size_);
    }
  }

  return size;
}

/**
 * checks if we've passed through all the data and need to reset
 * @tparam T
 * @return
 */
template <typename T>
bool W2VLoader<T>::IsDone() const
{
  if (current_sentence_ >= data_.size())
  {
    return true;
  }
  else if (current_sentence_ >= data_.size() - 1)  // In the last sentence
  {
    if (current_word_ >= data_.at(current_sentence_).size() - window_size_)
    {
      return true;
    }
  }
  return false;
}

/**
 * resets word cursors and re randomises negative sampling
 * @tparam T
 */
template <typename T>
void W2VLoader<T>::Reset()
{
  current_sentence_ = 0;
  current_word_     = 0;
  rng_.Seed(1337);
  unigram_table_.Reset();
}

/**
 * Remove words that appears less than MIN times. operation is destructive
 * @tparam T
 * @param min
 */
template <typename T>
void W2VLoader<T>::RemoveInfrequent(SizeType min)
{
  W2VLoader new_loader(window_size_, negative_samples_, mode_);
  std::map<SizeType, std::pair<std::string, SizeType>> reverse_vocab;
  for (auto const &kvp : vocab_.data)
  {
    reverse_vocab[kvp.second.first] = std::make_pair(kvp.first, kvp.second.second);
  }
  for (auto const &sentence : data_)
  {
    std::string s;
    for (auto const &word : sentence)
    {
      if (reverse_vocab[word].second >= min)
      {
        s += reverse_vocab[word].first + " ";
      }
    }
    new_loader.BuildVocab(s);
  }
  data_       = std::move(new_loader.data_);
  vocab_.data = std::move(new_loader.vocab_.data);
}

/**
 * initialises the unigram table for negative frequency based sampling
 * @tparam T
 */
template <typename T>
void W2VLoader<T>::InitUnigramTable()
{
  std::vector<SizeType> frequencies(vocab_size());
  for (auto const &kvp : vocab_.data)
  {
    frequencies[kvp.second.first] = kvp.second.second;
  }
  unigram_table_.Reset(frequencies, 1e8);
}

/**
 * gets the next set of data
 * @tparam T
 * @param t
 * @return
 */
template <typename T>
void W2VLoader<T>::GetNext(ReturnType &ret)
{
  // the current word should start from position that allows full context window
  if (current_word_ < window_size_)
  {
    current_word_ = window_size_;
  }

  // select random window size
  SizeType dynamic_size = rng_() % window_size_ + 1;

  // for the interested one word
  ret.first.Set(0, 0, T(data_[current_sentence_][current_word_]));

  // set the context samples
  for (SizeType i = 0; i < dynamic_size; ++i)
  {
    ret.second.at(0).Set(i, 0, T(data_[current_sentence_][current_word_ - i - 1]));
    ret.second.at(0).Set(i + dynamic_size, 0, T(data_[current_sentence_][current_word_ + i + 1]));
  }

  // denote the unused part of the window
  for (SizeType i = (dynamic_size * 2); i < ret.second.at(0).size(); ++i)
  {
    ret.second.at(0)(i, 0) = WindowContextUnused;
  }

  // negative sampling
  for (SizeType i = 1; i < negative_samples_; ++i)
  {
    SizeType neg_sample;
    bool     success =
        unigram_table_.SampleNegative(static_cast<SizeType>(ret.first(0, 0)), neg_sample);
    if (success)
    {
      ret.first(i, 0) = static_cast<T>(neg_sample);
    }
    else
    {
      throw std::runtime_error(
          "unigram table timed out looking for a negative sample. check window size for sentence "
          "length and that data loaded correctly.");
    }
  }

  current_word_++;
  // check if the word is window size away form either end of the sentence
  if (current_word_ >=
      data_.at(current_sentence_).size() -
          window_size_)  // the current word end when a full context window can be allowed
  {
    current_word_ =
        window_size_;  // the current word start from position that allows full context window
    current_sentence_++;
  }
}

template <typename T>
typename W2VLoader<T>::ReturnType W2VLoader<T>::GetNext()
{
  ReturnType p(label_, {target_});
  GetNext(p);
  return p;
}

/**
 * Adds a dataset to the dataloader
 * @tparam T
 * @param s input string containing all the text
 * @return bool indicates success
 */
template <typename T>
bool W2VLoader<T>::BuildVocab(std::string const &s)
{
  std::vector<SizeType> indexes = StringsToIndices(PreprocessString(s));
  if (indexes.size() >=
      2 * window_size_ + 1)  // each sentence stored in the data_ are guaranteed to have minimum
                             // length to handle window_size context sampling
  {
    data_.push_back(std::move(indexes));
    return true;
  }
  return false;
}

/**
 *
 * @tparam T
 * @param filename
 */
template <typename T>
void W2VLoader<T>::SaveVocab(std::string const &filename)
{
  vocab_.Save(filename);
}

/**
 *
 * @tparam T
 * @param filename
 */
template <typename T>
void W2VLoader<T>::LoadVocab(std::string const &filename)
{
  vocab_.Load(filename);
}

/**
 * get size of the vocab
 * @tparam T
 * @return
 */
template <typename T>
math::SizeType W2VLoader<T>::vocab_size() const
{
  return vocab_.data.size();
}

/**
 * export the vocabulary by reference
 * @tparam T
 * @return
 */
template <typename T>
typename W2VLoader<T>::VocabType const &W2VLoader<T>::vocab() const
{
  return vocab_;
}

/**
 * helper method for retrieving a word given its index in vocabulary
 * @tparam T
 * @param index
 * @return
 */
template <typename T>
std::string W2VLoader<T>::WordFromIndex(SizeType index) const
{
  return vocab_.WordFromIndex(index);
}

/**
 * helper method for retrieving word index given a word
 * @tparam T
 * @param word
 * @return
 */
template <typename T>
typename W2VLoader<T>::SizeType W2VLoader<T>::IndexFromWord(std::string const &word) const
{
  return vocab_.IndexFromWord(word);
}

template <typename T>
typename W2VLoader<T>::SizeType W2VLoader<T>::window_size()
{
  return window_size_;
}

/**
 * converts string to indices and inserts into vocab as necessary
 * @tparam T
 * @param strings
 * @return
 */
template <typename T>
std::vector<math::SizeType> W2VLoader<T>::StringsToIndices(std::vector<std::string> const &strings)
{
  std::vector<SizeType> indexes;
  if (strings.size() >= 2 * window_size_ + 1)  // Don't bother processing too short inputs
  {
    indexes.reserve(strings.size());
    for (std::string const &s : strings)
    {
      auto value = vocab_.data.insert(
          std::make_pair(s, std::make_pair((SizeType)(vocab_.data.size() + 1), 0)));
      indexes.push_back((*value.first).second.first);
      value.first->second.second++;
    }
  }
  return indexes;
}

/**
 * Preprocesses a string turning it into a vector of words
 * @tparam T
 * @param s
 * @return
 */
template <typename T>
std::vector<std::string> W2VLoader<T>::PreprocessString(std::string const &s)
{
  std::string result;
  result.reserve(s.size());
  for (auto const &c : s)
  {
    result.push_back(std::isalpha(c) ? (char)std::tolower(c) : ' ');
  }

  std::string              word;
  std::vector<std::string> words;
  for (std::stringstream ss(result); ss >> word;)
  {
    words.push_back(word);
  }
  return words;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
