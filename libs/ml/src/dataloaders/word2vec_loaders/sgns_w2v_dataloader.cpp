//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"

#include "math/standard_functions/sqrt.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/exceptions/exceptions.hpp"

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 *
 * @param window_size the size of the context window (one side only)
 * @param negative_samples the number of total samples (all but one being negative)
 */
template <typename TensorType>
GraphW2VLoader<TensorType>::GraphW2VLoader(SizeType const &           window_size,
                                           SizeType const &           negative_samples,
                                           fixed_point::fp64_t const &freq_thresh,
                                           SizeType const &           max_word_count,
                                           SizeType const &           seed)
  : DataLoader<TensorType>()  // no random mode specified
  , current_sentence_(0)
  , current_word_(0)
  , window_size_(window_size)
  , negative_samples_(negative_samples)
  , freq_thresh_(freq_thresh)
  , max_word_count_(max_word_count)
{
  using DataType = typename TensorType::Type;

  // setup temporary buffers for training purpose
  input_words_  = TensorType({negative_samples * window_size_ * 2 + window_size_ * 2});
  output_words_ = TensorType({negative_samples * window_size_ * 2 + window_size_ * 2});
  output_words_buffer_ =
      fetch::math::Tensor<SizeType>({negative_samples * window_size_ * 2 + window_size_ * 2});
  labels_ = TensorType({negative_samples * window_size_ * 2 + window_size_ * 2 +
                        1});  // the extra 1 is for testing if label has ran out
  labels_.Fill(static_cast<DataType>(BufferPositionUnusedDataType));
  cur_sample_.first  = TensorType({1, 1});
  cur_sample_.second = {TensorType({1, 1}), TensorType({1, 1})};
  this->SetSeed(seed);
}

/**
 * calculate the compatible linear decay rate for learning rate with our optimiser.
 * @return
 */
template <typename TensorType>
fetch::math::SizeType GraphW2VLoader<TensorType>::EstimatedSampleNumber()
{
  auto                estimated_sample_number = fixed_point::fp64_t{0};
  fixed_point::fp64_t word_freq;
  auto                estimated_sample_number_per_word =
      static_cast<fixed_point::fp64_t>((window_size_ + 1) * (1 + negative_samples_));

  for (auto word_count : word_id_counts_)
  {
    word_freq =
        static_cast<fixed_point::fp64_t>(word_count) / static_cast<fixed_point::fp64_t>(size_);
    if (word_freq > freq_thresh_)
    {
      estimated_sample_number += static_cast<fixed_point::fp64_t>(word_count) *
                                 estimated_sample_number_per_word *
                                 fetch::math::Sqrt(freq_thresh_ / word_freq);
    }
    else
    {
      estimated_sample_number =
          estimated_sample_number +
          static_cast<fixed_point::fp64_t>(word_count) * estimated_sample_number_per_word;
    }
  }
  return static_cast<SizeType>(estimated_sample_number);
}

/**
 * reports the total size of the outputs iterating through the dataloader
 * @return
 */
template <typename TensorType>
math::SizeType GraphW2VLoader<TensorType>::Size() const
{
  return size_;
}

/**
 * checks if we've passed through all the data and need to reset
 * @return
 */
template <typename TensorType>
bool GraphW2VLoader<TensorType>::IsDone() const
{
  // Loader can't be done if current sentence is not last
  if (current_sentence_ < data_.size() - 1)
  {
    return false;
  }

  // check if the buffer is drained
  if (labels_.At(buffer_pos_) != BufferPositionUnusedDataType)
  {
    return false;
  }

  // Loader is done when current sentence is out of range
  if (current_sentence_ >= data_.size())
  {
    return true;
  }

  // Check if is last word of last sentence
  return current_word_ >= data_.at(current_sentence_).size() - window_size_;
}

/**
 * resets word cursors and re randomises negative sampling
 */
template <typename TensorType>
void GraphW2VLoader<TensorType>::Reset()
{
  using DataType = typename TensorType::Type;

  current_sentence_ = 0;
  current_word_     = 0;
  unigram_table_.ResetRNG();
  labels_.Fill(static_cast<DataType>(BufferPositionUnusedDataType));
  buffer_pos_ = 0;
  reset_count_++;
}

template <typename TensorType>
void GraphW2VLoader<TensorType>::SetTestRatio(fixed_point::fp32_t new_test_ratio)
{
  FETCH_UNUSED(new_test_ratio);
  throw exceptions::InvalidMode("Test set splitting is not supported for this dataloader.");
}

template <typename TensorType>
void GraphW2VLoader<TensorType>::SetValidationRatio(fixed_point::fp32_t new_validation_ratio)
{
  FETCH_UNUSED(new_validation_ratio);
  throw exceptions::InvalidMode("Validation set splitting is not supported for this dataloader.");
}

/**
 * check if the word is in vocabulary
 * @param word
 * @return
 */
template <typename TensorType>
bool GraphW2VLoader<TensorType>::WordKnown(std::string const &word) const
{
  return vocab_->IndexFromWord(word) != Vocab::UNKNOWN_WORD;
}

/**
 * Remove words that appears less than MIN times. operation is destructive
 * @param min
 */
template <typename TensorType>
void GraphW2VLoader<TensorType>::RemoveInfrequent(SizeType min)
{
  // remove infrequent words from vocab first
  auto old2new = vocab_->RemoveInfrequentWord(min);

  // create a new data_ for storing text
  std::vector<std::vector<SizeType>> new_data;
  SizeType                           new_size(0);
  std::vector<SizeType>              new_counts(vocab_size(), SizeType{0});

  std::vector<SizeType> new_sent_buffer;  // buffer for each sentence
  for (auto const &sentence : data_)
  {
    // clear the buffer for this new sentence
    new_sent_buffer.clear();

    // reserve every word that is not infrequent
    for (auto word_it : sentence)
    {
      if (old2new.find(word_it) != old2new.end())
      {  // if a word is in old2new, append the new id of it to the new sentence
        new_sent_buffer.push_back(old2new[word_it]);
      }
    }

    // if after we remove infrequent word, the sentence becomes too short, we need to remove the
    // sentence
    if (new_sent_buffer.size() <= 2 * window_size_)
    {
      vocab_->RemoveSentenceFromVocab(sentence);
      // N.B. for practical concerns, we do not further remove infrequent words
    }
    else
    {  // reserve the sentence if the sentence is still long enough
      new_data.push_back(new_sent_buffer);
      new_size += new_sent_buffer.size() - (2 * window_size_);
      for (SizeType ind : new_sent_buffer)
      {
        new_counts[ind] += 1;
      }
    }
  }
  data_           = new_data;
  size_           = new_size;
  word_id_counts_ = new_counts;
}

/**
 * Remove words that appears less than MIN times in the data, not vocab
 * @param min
 */
template <typename TensorType>
void GraphW2VLoader<TensorType>::RemoveInfrequentFromData(SizeType min)
{
  // create a new data_ for storing text
  std::vector<std::vector<SizeType>> new_data;
  SizeType                           new_size(0);
  std::vector<SizeType>              new_counts(vocab_size(), SizeType{0});
  std::vector<SizeType>              new_sent_buffer;  // buffer for each sentence

  for (auto sent_it : data_)
  {
    // clear the buffer for this new sentence
    new_sent_buffer.clear();

    // reserve every word that is not infrequent
    for (auto word_it : sent_it)
    {
      if (word_id_counts_[word_it] > min)
      {
        new_sent_buffer.push_back(word_it);
      }
    }

    if (new_sent_buffer.size() > 2 * window_size_)
    {
      // reserve the sentence if the sentence is still long enough
      new_data.push_back(new_sent_buffer);
      new_size += new_sent_buffer.size() - (2 * window_size_);
      for (SizeType ind : new_sent_buffer)
      {
        new_counts[ind] += 1;
      }
    }
  }

  data_           = new_data;
  size_           = new_size;
  word_id_counts_ = new_counts;
}
/**
 * initialises the unigram table for negative frequency based sampling
 */
template <typename TensorType>
void GraphW2VLoader<TensorType>::InitUnigramTable(SizeType size, bool use_vocab_frequencies)
{
  if (use_vocab_frequencies)
  {
    unigram_table_.ResetTable(vocab_->GetCounts(), size);
  }
  else  // use counts from data
  {
    unigram_table_.ResetTable(word_id_counts_, size);
  }
}
/**
 * Buffer for the next word and relevent samples
 */
template <typename TensorType>
void GraphW2VLoader<TensorType>::BufferNextSamples()
{
  using DataType = typename TensorType::Type;

  // reset the index to buffer
  buffer_pos_ = 0;

  // the current word should start from position that allows full context window
  if (current_word_ < window_size_)
  {
    current_word_ = window_size_;
  }

  // subsample too frequent word
  while (true)
  {
    auto word_freq = static_cast<fixed_point::fp64_t>(
                         word_id_counts_[data_.at(current_sentence_).at(current_word_)]) /
                     static_cast<fixed_point::fp64_t>(size_);
    auto random_var =
        this->rand.template AsType<fixed_point::fp64_t>();  // random variable between 0-1
    if (random_var < fixed_point::fp64_t{1} - fetch::math::Sqrt(freq_thresh_ / word_freq))
    {  // subsample for a cumulative prob of 1 - sqrt(thresh/freq) // N.B. if word_freq <
      // freq_thresh, then subsampling would not happen
      // store data in case this is the last word
      auto prev_word     = current_word_;
      auto prev_sentence = current_sentence_;
      // move to the next word
      current_word_++;
      // check if the word is window size away from either end of the sentence
      if (current_word_ >=
          data_.at(current_sentence_).size() -
              window_size_)  // the current word end when a full context window can be allowed
      {
        current_word_ =
            window_size_;  // the current word start from position that allows full context window
        current_sentence_++;
      }
      if (IsDone() || current_sentence_ >= data_.size())
      {  // revert to the previous word if this is the last word
        current_word_     = prev_word;
        current_sentence_ = prev_sentence;
        break;
      }
    }
    else
    {
      break;
    }
  }

  // select random window size
  SizeType dynamic_size = SizeType{this->rand()} % window_size_ + 1;

  // for the interested one word
  auto cur_word_id = DataType(data_.at(current_sentence_).at(current_word_));

  // set up a counter to add samples to buffer
  SizeType counter = 0;

  input_words_.Fill(cur_word_id);
  labels_.Fill(static_cast<DataType>(BufferPositionUnusedDataType));
  output_words_.Fill(static_cast<DataType>(BufferPositionUnusedDataType));
  output_words_buffer_.Fill(BufferPositionUnusedSizeType);

  // set the context samples
  for (SizeType i = 0; i < dynamic_size; ++i)
  {
    output_words_buffer_.At(counter) = data_.at(current_sentence_).at(current_word_ - i - 1);
    output_words_.At(counter) =
        static_cast<DataType>(data_.at(current_sentence_).at(current_word_ - i - 1));
    labels_.At(counter) = DataType{1};
    counter++;

    output_words_buffer_.At(counter) = data_.at(current_sentence_).at(current_word_ + i + 1);
    output_words_.At(counter) =
        static_cast<DataType>(data_.at(current_sentence_).at(current_word_ + i + 1));
    labels_.At(counter) = DataType{1};
    counter++;
  }

  // negative sampling for every context word
  SizeType neg_sample;
  for (SizeType i = 0; i < negative_samples_ * dynamic_size * 2; ++i)
  {
    if (!unigram_table_.SampleNegative(output_words_buffer_, neg_sample))
    {
      throw exceptions::Timeout(
          "unigram table timed out looking for a negative sample. check window size for sentence "
          "length and that data loaded correctly.");
    }

    output_words_.At(counter) = DataType(neg_sample);
    labels_.At(counter)       = 0;
    counter++;
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
 * @return
 */
template <typename TensorType>
typename GraphW2VLoader<TensorType>::ReturnType GraphW2VLoader<TensorType>::GetNext()
{
  using DataType = typename TensorType::Type;

  DataType input_word, output_word;

  DataType label = labels_.At(buffer_pos_);  // check if we have drained the buffer, either no more
  // valid data, or goes out of bound
  if (label == BufferPositionUnusedDataType)
  {
    BufferNextSamples();
  }

  // get input, output and label for the cur sample
  input_word  = input_words_.At(buffer_pos_);
  output_word = output_words_.At(buffer_pos_);
  label       = labels_.At(buffer_pos_);

  // move buffer index to next one
  buffer_pos_++;

  cur_sample_.first.At(0, 0)        = label;
  cur_sample_.second.at(0).At(0, 0) = input_word;
  cur_sample_.second.at(1).At(0, 0) = output_word;

  return cur_sample_;
}

template <typename TensorType>
bool GraphW2VLoader<TensorType>::AddData(std::vector<TensorType> const &input,
                                         TensorType const &             label)
{
  FETCH_UNUSED(input);
  FETCH_UNUSED(label);
  throw exceptions::InvalidMode("AddData not implemented for word 2 vec dataloader");
}

/**
 * Adds a dataset to the dataloader
 * @param s input string containing all the text
 * @return bool indicates success
 */
template <typename TensorType>
void GraphW2VLoader<TensorType>::BuildVocabAndData(std::vector<std::string> const &sents,
                                                   SizeType min_count, bool build_data)
{
  // build vocab from sentences
  std::cout << "building vocab and data" << std::endl;
  for (auto const &s : sents)
  {
    // make sure the max_word_count is not exceeded, and also the remain words space allows a
    // meaningful sentence
    if (size_ >= max_word_count_ - 2 * window_size_)
    {
      return;
    }

    // if we can take in more words, then start processing words
    std::vector<std::string> preprocessed_string = PreprocessString(s, max_word_count_ - size_);

    if (preprocessed_string.size() <= 2 * window_size_)
    {  // dispose short sentences before we create vocabulary and count frequency: if we are not
      // gonna
      // train on it, the sentence does not exist
      continue;
    }

    std::vector<SizeType> indices = vocab_->PutSentenceInVocab(preprocessed_string);
    if (build_data)
    {
      data_.push_back(indices);
      size_ += indices.size() - (2 * window_size_);
      word_id_counts_.resize(vocab_size(), SizeType{0});
      for (SizeType ind : indices)
      {
        word_id_counts_[ind] += 1;
      }
    }
  }

  if (min_count > 0)
  {
    // remove infrequent words from data
    std::cout << "Removing infrequent words from vocab and data" << std::endl;
    RemoveInfrequent(min_count);
  }

  if (build_data)
  {
    // initialise unigram
    std::cout << "Initialising unigram" << std::endl;
    InitUnigramTable();
  }
}

template <typename TensorType>
void GraphW2VLoader<TensorType>::BuildData(std::vector<std::string> const &sents,
                                           SizeType                        min_count)
{
  assert(vocab_->GetWordCount() >= 0);

  // build vocab from sentences
  std::cout << "building data " << std::endl;
  for (auto const &sent : sents)
  {
    // make sure the max_word_count is not exceeded, and also the remain words space allows a
    // meaningful sentence
    if (size_ >= max_word_count_ - 2 * window_size_)
    {
      return;
    }

    // if we can take in more words, then start processing words
    std::vector<std::string> preprocessed_string = PreprocessString(sent, max_word_count_ - size_);

    if (preprocessed_string.size() <= 2 * window_size_)
    {  // dispose short sentences before we create vocabulary and count frequency
      continue;
    }

    std::vector<SizeType> indices{};
    for (std::string const &word : preprocessed_string)
    {
      // some words will be missing from the vocab because of infrequency
      SizeType word_ind = vocab_->IndexFromWord(word);
      if (word_ind != Vocab::UNKNOWN_WORD)
      {
        indices.push_back(word_ind);  // update the word in the sentence to index
      }
    }

    data_.push_back(indices);
    size_ += indices.size() - (2 * window_size_);
    word_id_counts_.resize(vocab_size(), SizeType{0});
    for (SizeType ind : indices)
    {
      word_id_counts_[ind] += 1;
    }
  }

  if (min_count > 0)
  {
    // remove infrequent words from data
    std::cout << "Removing infrequent words from data" << std::endl;
    RemoveInfrequentFromData(min_count);
  }

  // initialise unigram
  std::cout << "Initialising unigram" << std::endl;
  InitUnigramTable(1e8, false);
}

/**
 * Save the vocabulary to file
 * @param filename
 */
template <typename TensorType>
void GraphW2VLoader<TensorType>::SaveVocab(std::string const &filename)
{
  vocab_->Save(filename);
}

/**
 * Load vocabulary from file
 * @param filename
 */
template <typename TensorType>
void GraphW2VLoader<TensorType>::LoadVocab(std::string const &filename)
{
  vocab_->Load(filename);
}

/**
 * get size of the vocab
 * @return
 */
template <typename TensorType>
math::SizeType GraphW2VLoader<TensorType>::vocab_size() const
{
  return vocab_->GetVocabCount();
}

/**
 * export the vocabulary by reference
 * @return
 */
template <typename TensorType>
std::shared_ptr<typename GraphW2VLoader<TensorType>::VocabType> const &
GraphW2VLoader<TensorType>::GetVocab() const
{
  return vocab_;
}

/**
 * helper method for retrieving a word given its index in vocabulary
 * @param index
 * @return
 */
template <typename TensorType>
std::string GraphW2VLoader<TensorType>::WordFromIndex(math::SizeType index) const
{
  return vocab_->WordFromIndex(index);
}

/**
 * helper method for retrieving word index given a word
 * @param word
 * @return
 */
template <typename TensorType>
typename GraphW2VLoader<TensorType>::SizeType GraphW2VLoader<TensorType>::IndexFromWord(
    std::string const &word) const
{
  return vocab_->IndexFromWord(word);
}

template <typename TensorType>
typename GraphW2VLoader<TensorType>::SizeType GraphW2VLoader<TensorType>::WindowSize()
{
  return window_size_;
}

/**
 * Preprocesses a string turning it into a vector of words
 * @param s
 * @return
 */
template <typename TensorType>
std::vector<std::string> GraphW2VLoader<TensorType>::PreprocessString(std::string const &s,
                                                                      SizeType length_limit)
{
  std::string result;
  result.reserve(s.size());
  for (auto const &c : s)
  {
    result.push_back(std::isalpha(c) != 0 ? static_cast<char>(std::tolower(c)) : ' ');
  }

  std::string              word;
  std::vector<std::string> words;
  SizeType                 word_count = 0;
  for (std::stringstream ss(result); ss >> word;)
  {
    if (word_count >= length_limit)
    {
      break;
    }
    words.push_back(word);
    word_count++;
  }
  return words;
}

template <typename TensorType>
void GraphW2VLoader<TensorType>::UpdateCursor()
{
  if (this->mode_ != DataLoaderMode::TRAIN)
  {
    throw exceptions::InvalidMode("Other mode than training not supported.");
  }
}

template <typename TensorType>
bool GraphW2VLoader<TensorType>::IsModeAvailable(DataLoaderMode mode)
{
  return mode == DataLoaderMode::TRAIN;
}

template <typename TensorType>
byte_array::ConstByteArray GraphW2VLoader<TensorType>::GetVocabHash()
{
  return vocab_->GetVocabHash();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// TODO(ML-438)
// template class GraphW2VLoader<math::Tensor<std::int8_t>>;
// template class GraphW2VLoader<math::Tensor<std::int16_t>>;
template class GraphW2VLoader<math::Tensor<std::int32_t>>;
template class GraphW2VLoader<math::Tensor<std::int64_t>>;
template class GraphW2VLoader<math::Tensor<float>>;
template class GraphW2VLoader<math::Tensor<double>>;
template class GraphW2VLoader<math::Tensor<fixed_point::fp32_t>>;
template class GraphW2VLoader<math::Tensor<fixed_point::fp64_t>>;
template class GraphW2VLoader<math::Tensor<fixed_point::fp128_t>>;

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
