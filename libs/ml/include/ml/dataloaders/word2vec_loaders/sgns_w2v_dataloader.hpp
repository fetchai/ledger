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
#include "math/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/dataloaders/word2vec_loaders/unigram_table.hpp"
#include "ml/dataloaders/word2vec_loaders/vocab.hpp"

#include <map>
#include <string>

namespace fetch {
namespace ml {
namespace dataloaders {

template <typename T>
class W2VLoader : public DataLoader<fetch::math::Tensor<T>, fetch::math::Tensor<T>>
{
public:
    const T BufferPositionUnused = -1;
    
  using LabelType = fetch::math::Tensor<T>;
  using DataType  = fetch::math::Tensor<T>;

  using SizeType   = fetch::math::SizeType;
  using VocabType  = Vocab;
  using ReturnType = std::pair<LabelType, std::vector<DataType>>;

  W2VLoader(SizeType window_size, SizeType negative_samples, T freq_thresh, bool mode);

  bool       IsDone() const override;
  void       Reset() override;
  void       RemoveInfrequent(SizeType min);
  void       InitUnigramTable(SizeType size=1e8);
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
  T                                          freq_thresh_;
  VocabType                                  vocab_;
  std::vector<std::vector<SizeType>>         data_;
  fetch::random::LinearCongruentialGenerator rng_;
  UnigramTable                               unigram_table_;
  bool                                       mode_;
  SizeType                                   size_ = 0;

  // temporary sample and labels for buffering samples
  DataType                 input_words_, output_words_, labels_;
  SizeType                 buffer_pos_ = 0;
  ReturnType               cur_sample_;

  std::vector<SizeType>    SentenceToIndices(std::vector<std::string> const &strings);
  std::vector<std::string> PreprocessString(std::string const &s);
  void                     BufferNextSamples(SizeType seed = 123456789);
  void                     Update();
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
W2VLoader<T>::W2VLoader(SizeType window_size, SizeType negative_samples, T freq_thresh, bool mode)
  : DataLoader<LabelType, DataType>(false)  // no random mode specified
  , current_sentence_(0)
  , current_word_(0)
  , window_size_(window_size)
  , negative_samples_(negative_samples)
  , freq_thresh_(freq_thresh)
  , mode_(mode)
{
  // setup temporary buffers for training purpose
  input_words_ = DataType({negative_samples*window_size_*2 + window_size_*2});
  output_words_ = DataType({negative_samples*window_size_*2 + window_size_*2});
  labels_ = DataType({negative_samples*window_size_*2 + window_size_*2 + 1}); // the extra 1 is for testing if label has ran out
  labels_.Fill(BufferPositionUnused);
  cur_sample_.first = DataType({1, 1});
  cur_sample_.second = {DataType({1, 1}), DataType({1, 1})};
}

/**
 * reports the total size of the outputs iterating through the dataloader
 * @tparam T
 * @return
 */
template <typename T>
math::SizeType W2VLoader<T>::Size() const
{
    return size_;
}

/**
 * update stats of data in dataloader, including: size_,
 */
template <typename T>
void W2VLoader<T>::Update(){
    // update data set size
    SizeType size(0);
    for (auto const &s : data_)
    {
        if ((SizeType)s.size() > (2 * window_size_))
        {
            size += (SizeType)s.size() - (2 * window_size_);
        }
    }
    size_ = size;
}

/**
 * checks if we've passed through all the data and need to reset
 * @tparam T
 * @return
 */
template <typename T>
bool W2VLoader<T>::IsDone() const
{
    if (current_sentence_ < data_.size() - 1) {
        return false;
    }
    if((current_sentence_ == data_.size() - 1) && (current_word_ < data_.at(current_sentence_).size() - window_size_)) {
        return false;
    }
    return true;
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

    // update vocab
    vocab_.Update();

    // update dl
    Update();
}

/**
 * initialises the unigram table for negative frequency based sampling
 * @tparam T
 */
template <typename T>
void W2VLoader<T>::InitUnigramTable(SizeType size)
{
  std::vector<SizeType> frequencies(vocab_size());
  for (auto const &kvp : vocab_.data)
  {
    frequencies[kvp.second.first] = kvp.second.second;
  }
  unigram_table_.Reset(frequencies, size);
}
/**
 * Buffer for the next word and relevent samples
 * @tparam T
 * @param seed
 */
template <typename T>
void W2VLoader<T>::BufferNextSamples(SizeType seed)
{

    // reset the index to buffer
    buffer_pos_ = 0;

  // the current word should start from position that allows full context window
  if (current_word_ < window_size_)
  {
    current_word_ = window_size_;
  }

  // subsample too frequent word
  fetch::random::LaggedFibonacciGenerator<> lfg_(seed);
  while(true){
      T word_freq = static_cast<T>(vocab_.reverse_data[data_.at(current_sentence_).at(current_word_)].second) / static_cast<T>(vocab_.total_count);
      T random_var = static_cast<T>(lfg_.AsDouble()); // random variable between 0-1
      if(random_var < 1 - fetch::math::Sqrt(freq_thresh_ / word_freq)){ // subsample for a cumulative prob of 1 - sqrt(thresh/freq) // N.B. if word_freq < freq_thresh, then subsampling would not happen
          // store data in case this is the last word
          auto prev_word = current_word_;
          auto prev_sentence = current_sentence_;
          // move to the next word
          current_word_++;
          // check if the word is window size away form either end of the sentence
          if (current_word_ >= data_.at(current_sentence_).size() - window_size_)  // the current word end when a full context window can be allowed
          {
              current_word_ = window_size_;  // the current word start from position that allows full context window
              current_sentence_++;
          }
          if(IsDone()){ // revert to the previous word if this is the last word
              current_word_ = prev_word;
              current_sentence_ = prev_sentence;
              break;
          }
      }else{
          break;
      }
  }

  // select random window size
  SizeType dynamic_size = rng_() % window_size_ + 1;

  // for the interested one word
  T cur_word_id = T(data_.at(current_sentence_).at(current_word_));
    
  // set up a counter to add samples to buffer
  SizeType counter = 0;
  
  // reset all three buffers
  input_words_.Fill(cur_word_id);
    labels_.Fill(BufferPositionUnused);
  
  // set the context samples
  for (SizeType i = 0; i < dynamic_size; ++i)
  {
    output_words_.At(counter) = T(data_.at(current_sentence_).at(current_word_ - i - 1));
    labels_.At(counter) = T(1);
    counter++;
    
      output_words_.At(counter) = T(data_.at(current_sentence_).at(current_word_ + i + 1));
      labels_.At(counter) = T(1);
    counter++;
  }

  // negative sampling for every context word
  SizeType neg_sample;
  for (SizeType i = 0; i < negative_samples_ * dynamic_size * 2; ++i)
  {
    if (!unigram_table_.SampleNegative(static_cast<SizeType>(cur_word_id), neg_sample)){
        throw std::runtime_error(
                "unigram table timed out looking for a negative sample. check window size for sentence "
                "length and that data loaded correctly.");
    }else{
      output_words_.At(counter) = T(neg_sample);
      labels_.At(counter) = 0;
      counter++;
    }
  }

  // move the index to the next word
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

/**
 * get next one sample from buffer
 * @tparam T
 * @return
 */
template <typename T>
typename W2VLoader<T>::ReturnType W2VLoader<T>::GetNext()
{
    T input_word, output_word;

    T label =  labels_.At(buffer_pos_); // check if we have drained the buffer, either no more valid data, or goes out of bound
  if (label == BufferPositionUnused)
  {
      BufferNextSamples();
  }

    // get input, output and label for the cur sample
    input_word = input_words_.At(buffer_pos_);
    output_word = output_words_.At(buffer_pos_);
    label =  labels_.At(buffer_pos_);

  // move buffer index to next one
  buffer_pos_++;

  cur_sample_.first.At(0, 0) = label;
  cur_sample_.second.at(0).At(0, 0) = input_word;
  cur_sample_.second.at(1).At(0, 0) = output_word;

  return cur_sample_;
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
        std::vector<SizeType> indices = SentenceToIndices(preprocessed_string);
        data_.push_back(indices);

        // update stats in vocab_ and dl
        vocab_.Update();
        Update();
        return true;
    }
}

/**
 * Save the vocabulary to file
 * @tparam T
 * @param filename
 */
template <typename T>
void W2VLoader<T>::SaveVocab(std::string const &filename)
{
  vocab_.Save(filename);
}

/**
 * Load vocabulary from file
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
std::vector<math::SizeType> W2VLoader<T>::SentenceToIndices(
    std::vector<std::string> const &strings)
{
    assert(strings.size() >= 2 * window_size_ + 1);  // All input are guaranteed to be long enough for the training

    std::vector<SizeType> indices;
    indices.reserve(strings.size());

    for (std::string const &s : strings){
        auto value = vocab_.data.insert(std::make_pair(s, std::make_pair(static_cast<SizeType>(vocab_.data.size()), 0))); // insert a word info block into vocab_.data
        indices.push_back((*value.first).second.first); // update the word in the sentence to index
        value.first->second.second++; // adding up count for word s
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
