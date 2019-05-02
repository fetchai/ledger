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
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/text_loader.hpp"

#include <algorithm>  // random_shuffle
#include <numeric>    // std::iota

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename T>
struct TextParams
{

public:
  using SizeType = typename T::SizeType;

  TextParams(bool fw = false)
    : full_window(fw){};

  SizeType min_sentence_length = 2;  // minimum number of words in a sentence
  SizeType max_sentences       = 0;  // maximum number of sentences in training set

  SizeType n_data_buffers = 1;  // number of word indices to return when called
  SizeType window_size    = 0;  // the size of the context window (one-sided)

  bool full_window = false;  // whether we may only index words with a full window either side

  // optional processing
  bool discard_frequent = false;  // discard frequent words

  // discard params
  double discard_threshold = 0.00001;  // random discard probability threshold
};

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class BasicTextLoader : public TextLoader<T>, public DataLoader<T, typename T::SizeType>
{
public:
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

  explicit BasicTextLoader(TextParams<ArrayType> const &p, SizeType seed = 123456789);

  // overloaded member from dataloader
  std::pair<T, SizeType>         GetNext() override;
  virtual std::pair<T, SizeType> GetRandom();
  SizeType                       Size() const override;
  virtual bool                   IsDone() const override;
  void                           Reset() override;

  virtual std::pair<T, SizeType> GetAtIndex(SizeType idx);
  SizeType                       GetDiscardCount();
  bool                           AddData(std::string const &text) override;

protected:
  // params
  TextParams<ArrayType> p_;

  // random generators
  fetch::random::LaggedFibonacciGenerator<>  lfg_;
  fetch::random::LinearCongruentialGenerator lcg_;

  // containers for the data and labels
  ArrayType data_buffers_;

  // discard related containers and counts
  std::vector<std::vector<SizeType>> discards_;  // record of discarded words
  SizeType discard_sentence_idx_ = 0;  // keeps track of sentences already having discard applied
  SizeType discard_count_        = 0;  // total count of discarded (frequent) words

  // used for iterating through all examples incrementally
  SizeType              cursor_;      // indexes through data sequentially
  SizeType              ran_cursor_;  // indexes through data randomly
  std::vector<SizeType> ran_idx_;     // random indices container

  void     GetData(SizeType idx, ArrayType &ret) override;
  SizeType GetLabel(SizeType idx) override;

  SizeType GetWordOffsetFromWordIdx(SizeType word_idx) const;

private:
  bool cursor_set_     = false;  // whether cursor currently set to a valid position
  bool ran_cursor_set_ = false;  // whether random cursor currently set to a valid position

  void GetNextValidIndices();
  bool CheckValidIndex(SizeType idx);
  void DiscardFrequent();
  bool DiscardExample(SizeType word_frequency);
};

/**
 *
 * @tparam T
 */
template <typename T>
BasicTextLoader<T>::BasicTextLoader(TextParams<T> const &p, SizeType seed)
  : p_(p)
  , lfg_(seed)
  , lcg_(seed)
  , cursor_(0)
{
  assert(p_.min_sentence_length > 1);

  // If user specifies to use full windows, they dont need to specify minimum sentence length
  SizeType min_viable_sentnce;
  if (p_.full_window)
  {
    min_viable_sentnce = ((p_.window_size * 2) + 1);
    if (p_.min_sentence_length < min_viable_sentnce)
    {
      p_.min_sentence_length = min_viable_sentnce;
    }
  }

  this->min_sent_len_ = p_.min_sentence_length;
  this->max_sent_     = p_.max_sentences;

  data_buffers_ = T(p_.n_data_buffers);
}

/////////////////////////////////////
/// public member implementations ///
/////////////////////////////////////

/**
 * gets the next data point
 * @tparam T  Array type
 * @return  returns a pair of Array and Label
 */
template <typename T>
std::pair<T, typename BasicTextLoader<T>::SizeType> BasicTextLoader<T>::GetNext()
{
  GetNextValidIndices();
  if (cursor_set_)
  {
    return GetAtIndex(cursor_);
  }
  throw std::runtime_error("no valid cursor position set");
}

/**
 * gets the next data point from the randomised list
 * @tparam T  Array type
 * @return  returns a pair of Array and Label
 */
template <typename T>
std::pair<T, typename BasicTextLoader<T>::SizeType> BasicTextLoader<T>::GetRandom()
{
  GetNextValidIndices();

  if (ran_cursor_set_)
  {
    return GetAtIndex(ran_cursor_);
  }

  throw std::runtime_error("no valid cursor position set");
}

/**
 * Indicates total number of training data point if p_.full_window, otherwise indicates total number
 * of valid training indices (i.e. target words) which determines the minimum number of training
 * data points
 * @return
 */
template <typename T>
typename BasicTextLoader<T>::SizeType BasicTextLoader<T>::Size() const
{
  SizeType size(0);
  // for each sentence
  for (auto const &s : this->data_)
  {
    // not strictly necessary to check this, since text_loader won't add too short sentences anyway
    if ((SizeType)s.size() >= p_.min_sentence_length)
    {
      if (p_.full_window)
      {
        size += (SizeType)s.size() - (p_.min_sentence_length - 1);
      }
      else
      {
        size += (SizeType)s.size();
      }
    }
  }
  size -= discard_count_;

  return size;
}

/**
 * Returns whether we've iterated through all the data
 * @return
 */
template <typename T>
bool BasicTextLoader<T>::IsDone() const
{
  // check if no more valid positions until cursor reaches end
  if (p_.full_window)
  {
    return (this->data_.empty() ||
            (cursor_ >= (this->word_count_ - p_.window_size - discard_count_)));
  }
  else
  {
    return (this->data_.empty() || (cursor_ >= (this->word_count_ - discard_count_)));
  }
}

/**
 * resets the cursor for iterating through multiple epochs
 */
template <typename T>
void BasicTextLoader<T>::Reset()
{
  cursor_ = 0;

  // generate a new random sequence for random sampling
  // note that ran_idx_ is of size word_count_ not of size number of valid training pairs
  ran_idx_ = std::vector<SizeType>(this->TextLoader<T>::Size());
  std::iota(ran_idx_.begin(), ran_idx_.end(), 0);
  std::random_shuffle(ran_idx_.begin(), ran_idx_.end());
  assert(ran_idx_.size() == this->word_count_);

  // recompute which words should be ignored based on their frequency
  DiscardFrequent();

  // assign the cursors to their first valid position
  GetNextValidIndices();
}

/**
 * Gets the data at the specified word index
 * @param idx the word index (i.e. the word position in the entire training set)
 * @return  returns a pair of output buffer and label (i.e. X & Y)
 */
template <typename T>
std::pair<T, typename BasicTextLoader<T>::SizeType> BasicTextLoader<T>::GetAtIndex(SizeType idx)
{
  // pull data from multiple data buffers into single output buffer
  GetData(idx, data_buffers_);

  SizeType label = GetLabel(idx);

  // increment the cursor find the next valid position
  ++cursor_;
  if (cursor_ < this->TextLoader<T>::Size())
  {
    ran_cursor_ = ran_idx_.at(cursor_);
  }

  auto tmp = std::make_pair(data_buffers_, label);

  return tmp;
}

/**
 * Reports the total number of 'discarded' words. Note that words are never really discarded, only
 * masked
 * @tparam T  Array Type
 * @return the count of discarded/masked words
 */
template <typename T>
typename BasicTextLoader<T>::SizeType BasicTextLoader<T>::GetDiscardCount()
{
  return discard_count_;
}

/**
 * Adds text to training data
 * @param training_data
 */
template <typename T>
bool BasicTextLoader<T>::AddData(std::string const &text)
{
  bool success = TextLoader<T>::AddData(text);

  // don't reset if no need data added!
  if (success)
  {
    this->Reset();
  }

  return success;
}

////////////////////////////////////////
/// protected member implementations ///
////////////////////////////////////////

/**
 * For the Basic dataloader GetData simply returns the word at the word index given. Inheriting data
 * loaders will usually override this method
 * @tparam T
 * @param idx
 * @param ret
 */
template <typename T>
void BasicTextLoader<T>::GetData(typename BasicTextLoader<T>::SizeType idx, T &ret)
{
  assert(p_.n_data_buffers == 1);
  SizeType sentence_idx = this->word_idx_sentence_idx.at(idx);
  SizeType word_idx     = this->GetWordOffsetFromWordIdx(idx);
  ret.At(0)             = DataType(this->data_.at(sentence_idx).at(word_idx));
}

/**
 * The basic text loader isn't useful for training, but implementing a GetLabel method
 * allows us to make it concrete and then run common unit tests
 * @tparam T ArrayType
 * @param idx word index
 * @return dummy label
 */
template <typename T>
typename BasicTextLoader<T>::SizeType BasicTextLoader<T>::GetLabel(
    typename BasicTextLoader<T>::SizeType /*idx*/)
{
  return 1;
}

/**
 * helper function for identifying the position of a word in the sentence
 * if you only have the word idx
 */
template <typename T>
typename BasicTextLoader<T>::SizeType BasicTextLoader<T>::GetWordOffsetFromWordIdx(
    typename BasicTextLoader<T>::SizeType word_idx) const
{
  // first get the sentence index
  SizeType word_offset;
  SizeType sentence_idx = this->word_idx_sentence_idx.at(word_idx);

  // if 0 - the word index is the word offset
  if (sentence_idx == 0)
  {
    word_offset = word_idx;
  }

  // if not - reduce the word index until we select the previous sentence
  // the difference must be the word offset
  else
  {
    SizeType cur_word_idx = word_idx - 1;
    SizeType word_idx_for_offset_zero;
    bool     not_found = true;
    while (not_found)
    {
      if (sentence_idx != this->word_idx_sentence_idx.at(cur_word_idx))
      {
        // first word in current sentence is therefore
        word_idx_for_offset_zero = cur_word_idx + 1;
        word_offset              = word_idx - word_idx_for_offset_zero;
        not_found                = false;
      }
      else
      {
        --cur_word_idx;
      }
    }
  }
  return word_offset;
}
//////////////////////////////////////
/// private member implementations ///
//////////////////////////////////////

/**
 * finds the next valid index
 * @tparam T  Array type
 * @param ret  the return index
 * @param random  whether to select indices at random
 * @return  returns a bool indicating failure condition
 */
template <typename T>
void BasicTextLoader<T>::GetNextValidIndices()
{
  cursor_set_     = false;
  ran_cursor_set_ = false;
  // when Adding data may be IsDone = True, in which case don't need to do anything
  if (!IsDone())
  {
    for (SizeType i = cursor_; i < this->word_count_; ++i)
    {
      if (!cursor_set_)
      {
        if (CheckValidIndex(i))
        {
          cursor_     = i;
          cursor_set_ = true;
        }
      }

      if (!ran_cursor_set_)
      {
        if (CheckValidIndex(ran_idx_.at(i)))
        {
          ran_cursor_     = ran_idx_.at(i);
          ran_cursor_set_ = true;
        }
      }

      if (cursor_set_ && ran_cursor_set_)
      {
        break;
      }
    }
  }
}

/**
 * checks if a data point may be indexed to create a training pair
 * @return
 */
template <typename T>
bool BasicTextLoader<T>::CheckValidIndex(SizeType idx)
{
  SizeType sentence_idx = this->word_idx_sentence_idx.at(idx);
  SizeType word_offset  = GetWordOffsetFromWordIdx(idx);
  bool     valid_idx    = true;

  // may only choose indices with a full window either side
  if (p_.full_window)
  {
    bool left_window  = (int(word_offset) - int(p_.window_size) >= 0);
    bool right_window = (word_offset + p_.window_size < this->data_.at(sentence_idx).size());
    if (!(left_window && right_window))
    {
      valid_idx = false;
    }
  }

  if (p_.discard_frequent)
  {
    if (discards_.at(sentence_idx).at(word_offset))
    {
      valid_idx = false;
    }
  }
  return valid_idx;
}

/**
 * discards words in training data set based on word frequency
 */
template <typename T>
void BasicTextLoader<T>::DiscardFrequent()
{
  if (p_.discard_frequent)
  {
    discards_.clear();
    discard_count_          = 0;
    discard_sentence_idx_   = 0;
    SizeType sentence_count = 0;

    // iterate through all sentences
    for (SizeType sntce_idx = 0; sntce_idx < this->data_.size(); sntce_idx++)
    {
      // again, not strictly necessary to check for this under current text_loader implementation
      if ((this->data_.at(sntce_idx).size() >= p_.min_sentence_length) &&
          (discard_sentence_idx_ + sentence_count < p_.max_sentences))
      {
        discards_.push_back({});

        // iterate through words in the sentence choosing which to discard
        for (SizeType i = 0; i < this->data_.at(sntce_idx).size(); i++)
        {
          if (DiscardExample(this->vocab_frequencies.at(this->data_.at(sntce_idx).at(i))))
          {
            discards_.at(discard_sentence_idx_ + sentence_count).push_back(1);
            ++discard_count_;
          }
          else
          {
            discards_.at(discard_sentence_idx_ + sentence_count).push_back(0);
          }
        }

        ++sentence_count;
      }
    }
    discard_sentence_idx_ += sentence_count;
  }
}

/**
 * according to mikolov et al. we should throw away examples with proportion to how common the
 * word is
 */
template <typename T>
bool BasicTextLoader<T>::DiscardExample(SizeType word_frequency)
{
  assert(word_frequency > 0);

  double word_probability = double(word_frequency) / double(this->word_count_);

  double prob_thresh = (std::sqrt(word_probability / p_.discard_threshold) + 1.0);
  prob_thresh *= (p_.discard_threshold / word_probability);
  double f = lfg_.AsDouble();

  if (f < prob_thresh)
  {
    return false;
  }
  return true;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
