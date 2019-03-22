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

#include "ml/dataloaders/text_loader.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "core/random/lcg.hpp"
#include "core/random/lfg.hpp"

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

  SizeType min_sentence_length = 0;  // minimum number of words in a sentence
  SizeType max_sentences       = 0;  // maximum number of sentences in training set


  SizeType n_data_buffers      = 0;  // number of data points to return when called
  SizeType window_size         = 0;  // the size of the context window (one-sided)
  SizeType max_idx_search      = 1000;

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
  virtual std::pair<T, SizeType> GetNext() override;
  virtual std::pair<T, SizeType> GetRandom() override;
  virtual SizeType               Size() const override;
  virtual bool                   IsDone() const override;
  virtual void                   Reset() override;

  virtual std::pair<T, SizeType> GetAtIndex(SizeType idx);
  SizeType GetDiscardCount();

protected:

  // params
  TextParams<ArrayType> p_;

  // random generators
  fetch::random::LaggedFibonacciGenerator<>  lfg_;
  fetch::random::LinearCongruentialGenerator lcg_;

  // containers for the data and labels
  std::vector<std::vector<SizeType>> data_buffers_;

  // discard related containers and counts
  std::vector<std::vector<SizeType>> discards_;  // record of discarded words
  SizeType discard_sentence_idx_ = 0;            // keeps track of sentences already having discard applied
  SizeType discard_count_     = 0;  // total count of discarded (frequent) words

  // used for iterating through all examples incrementally
  SizeType cursor_;                   // indexes through data
  bool     is_done_         = false;  // tracks progress of cursor
  std::vector<SizeType> ran_idx_;     // random indices container

  virtual void     GetData(SizeType idx, ArrayType &ret) override;
  virtual SizeType GetLabel(SizeType idx) override;
  void             AddData(std::string const &text) override;

  SizeType    GetWordOffsetFromWordIdx(SizeType word_idx) const;

private:
  bool        GetNextValidIndex(bool random, typename BasicTextLoader<T>::SizeType &ret);
  bool        CheckValidIndex(SizeType idx);
  void        DiscardFrequent();
  bool        DiscardExample(SizeType word_frequency);

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
  this->min_sent_len_ = p_.min_sentence_length;
  this->max_sent_len_ = p_.max_sentences;

  data_buffers_.resize(p_.n_data_buffers);

  Reset();
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
  SizeType idx;
  if(GetNextValidIndex(false, idx))
  {
    return GetAtIndex(idx);
  }
  else
  {
    throw std::runtime_error("exceeded maximum attempts to find valid index");
  }
}

/**
 * gets the next data point from the randomised list
 * @tparam T  Array type
 * @return  returns a pair of Array and Label
 */
template <typename T>
std::pair<T, typename BasicTextLoader<T>::SizeType> BasicTextLoader<T>::GetRandom()
{
  SizeType idx;
  if(GetNextValidIndex(true, idx))
  {
    return GetAtIndex(idx);
  }
  else
  {
    throw std::runtime_error("exceeded maximum attempts to find valid index");
  }
}

/**
 * Size method of dataloader implemented in textloader
 * @return
 */
template <typename T>
typename BasicTextLoader<T>::SizeType BasicTextLoader<T>::Size() const
{
  return TextLoader<T>::Size();
}

/**
 * Returns whether we've iterated through all the data
 * @return
 */
template <typename T>
bool BasicTextLoader<T>::IsDone() const
{
  return is_done_;
}

/**
 * resets the cursor for iterating through multiple epochs
 */
template <typename T>
void BasicTextLoader<T>::Reset()
{
  cursor_  = 0;
  is_done_ = false;

  // generate a new random sequence for random sampling
  ran_idx_ = std::vector<SizeType>(this->Size());
  std::iota(ran_idx_.begin(), ran_idx_.end(), 0);
  std::random_shuffle(ran_idx_.begin(), ran_idx_.end());

  // recompute which words should be ignored based on their frequency
  DiscardFrequent();
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
  T data_buffer(std::vector<SizeType>({1, p_.n_data_buffers}));
  GetData(idx, data_buffer);

  SizeType label;
  label = GetLabel(idx);

  cursor_++;

  return std::make_pair(data_buffer, label);
}

/**
 * Reports the total number of 'discarded' words. Note that words are never really discarded, only masked
 * @tparam T  Array Type
 * @return the count of discarded/masked words
 */
template <typename T>
typename BasicTextLoader<T>::SizeType BasicTextLoader<T>::GetDiscardCount()
{
  return discard_count_;
}

////////////////////////////////////////
/// protected member implementations ///
////////////////////////////////////////


/**
 * For the Basic dataloader GetData simply returns the word at the word index given. Inheriting data loaders
 * will usually override this method
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
 * Adds text to training data
 * @param training_data
 */
template <typename T>
void BasicTextLoader<T>::AddData(std::string const &text)
{
  TextLoader<T>::AddData(text);

  // have to reset to regenerate random indices for newly resized data
  this->Reset();
}

/**
 * The basic text loader isn't useful for training, but implementing a GetLabel method
 * allows us to make it concrete and then run common unit tests
 * @tparam T ArrayType
 * @param idx word index
 * @return dummy label
 */
template <typename T>
typename BasicTextLoader<T>::SizeType BasicTextLoader<T>::GetLabel(typename BasicTextLoader<T>::SizeType idx)
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
bool BasicTextLoader<T>::GetNextValidIndex(bool random, typename BasicTextLoader<T>::SizeType &ret)
{
  // loop until we find a non-discarded data point
  bool not_found = true;
  SizeType timeout_count = 0;
  while (not_found)
  {
    is_done_ = false;
    if (cursor_ >= this->word_count_)
    {
      Reset();
      is_done_ = true;
    }

    if(random ? CheckValidIndex(ran_idx_.at(cursor_)) : CheckValidIndex(cursor_))
    {
      not_found = false;
    }
    else
    {
      ++cursor_;
      ++timeout_count;
      if (timeout_count > p_.max_idx_search)
      {
        return false;
      }
    }
  }
  ret = random ? ran_idx_.at(cursor_) : cursor_;
  return true;
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
    SizeType sentence_count = 0;

    // iterate through all sentences
    for (SizeType sntce_idx = 0; sntce_idx < this->data_.size(); sntce_idx++)
    {
      if ((this->data_.at(sntce_idx).size() > p_.min_sentence_length) &&
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
