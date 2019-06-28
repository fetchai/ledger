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
  void       BufferNextSamples();
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

  std::vector<ReturnType> buffer;  // buffer for available samples
    // temporary sample and labels for buffering samples
    LabelType  label_one, label_zero;
    DataType   tmp_input, tmp_output;

  std::vector<SizeType>    StringsToIndices(std::vector<std::string> const &strings);
  std::vector<std::string> PreprocessString(std::string const &s);
};

/**
 *
 * @tparam T
 * @param window_size the size of the context window (one side only)
 * @param negative_samples the number of total samples (all but one being negat
 * SkipGramTextParams<ArrayType> sp = SetParams<ArrayType>();ive)
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
{
  // setup temporary buffers for training purpose
  label_one     = LabelType({1, 1});
  label_one(0, 0)  = 1;
  label_zero    = LabelType({1, 1});
  label_zero(0, 0) = 0;
  tmp_input     = DataType({1, 1});
  tmp_output    = DataType({1, 1});
}

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
  if (current_sentence_ == data_.size())
  {
    return true;
  }
  else if (current_sentence_ == data_.size() - 1)  // In the last sentence
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
    // remove infrequent words from vocab first
    vocab_.RemoveInfrequentWord(min);

    // compactify the vocabulary
    auto old2new = vocab_.Compactify();

    // create reverse vocab
    auto reverse_vocab = vocab_.GetReverseVocab();

    // create a new data_ for storing text
    std::vector<std::vector<SizeType>> new_data;

    for(auto sent_it = data_.begin(); sent_it != data_.end(); sent_it++){
        std::vector<SizeType> new_sent_buffer; // buffer for this sentence

        // reserve every word that is not infrequent
        for(auto word_it = sent_it->begin(); word_it != sent_it->end(); ++word_it){
            if(old2new.count(*word_it) > 0) { // if a word is in old2new, append it to the new sentence
                new_sent_buffer.push_back(*word_it);
            }
        }

        // if after we remove infrequent word, the sentence becomes too short, we need to remove the sentence
        if (new_sent_buffer.size() <= 2*window_size_){
            for(auto const &word_id : new_sent_buffer) { // reduce the count for all the word in this sentence
                vocab_.data[reverse_vocab[word_id].first].second -= 1;
            }
            // N.B. for practical concerns, we do not further remove infrequent words
        }else{ // reserve the sentence if the sentence is still long enough
            new_data.push_back(new_sent_buffer);
        }
    }
    data_ = new_data;
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
  unigram_table_.Reset(1e8, frequencies);
}

template <typename T>
void W2VLoader<T>::BufferNextSamples()
{
  // clear the buffer
  buffer.clear();

  // the current word should start from position that allows full context window
  if (current_word_ < window_size_)
  {
    current_word_ = window_size_;
  }

  // select random window size
  SizeType dynamic_size = rng_() % window_size_ + 1;

  // for the interested one word
  tmp_input(0, 0) = T(data_[current_sentence_][current_word_]);

  // set the context samples
  for (SizeType i = 0; i < dynamic_size; ++i)
  {
    tmp_output(0, 0) = T(data_[current_sentence_][current_word_ - i - 1]);
    buffer.push_back(ReturnType(label_one, {tmp_input, tmp_output}));

    tmp_output(0, 0) = T(data_[current_sentence_][current_word_ + i + 1]);
    buffer.push_back(ReturnType(label_one, {tmp_input, tmp_output}));
  }

  // negative sampling for every context word
  SizeType neg_sample;
  for (SizeType i = 1; i < negative_samples_ * window_size_ * 2; ++i)
  {
    bool success = unigram_table_.SampleNegative(SizeType(tmp_input(0, 0)), neg_sample);
    if (success)
    {
      tmp_output(0, 0) = T(neg_sample);
      buffer.push_back(ReturnType(label_zero, {tmp_input, tmp_output}));
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

  ReturnType output;
  if (buffer.size() == 0)
  {
    BufferNextSamples();
  }

  output = buffer.back();
  buffer.pop_back();

  return output;
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
    std::vector<std::string> preprocessed_string = PreprocessString(s);

    if(preprocessed_string.size() <= 2*window_size_){ // dispose short sentences before we create vocabulary and count frequency: if we are not gonna train on it, the sentence does not exist
        return false;
    }else{
        std::vector<SizeType> indices = StringsToIndices(preprocessed_string);
        data_.push_back(indices);
        return true;
    }
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
std::vector<math::SizeType> W2VLoader<T>::StringsToIndices(
    std::vector<std::string> const &strings)  // it is more like words to indices (each string is a word not a sentence)
{
    assert(strings.size() >= 2 * window_size_ + 1);  // All input are guaranteed to be long enough for the training

    std::vector<SizeType> indices;
    indices.reserve(strings.size());

    for (std::string const &s : strings){
        auto value = vocab_.data.insert(std::make_pair(s, std::make_pair(SizeType(vocab_.data.size()), 0))); // the first insertion should give frequency = 1 not 0!!!!!!!!
        indices.push_back((*value.first).second.first);
        value.first->second.second++;
    }
    return indices;
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
