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
#include "math/distance/cosine.hpp"
#include "ml/dataloaders/dataloader.hpp"

#include <algorithm>  // random_shuffle
#include <fstream>    // file streaming
#include <numeric>    // std::iota
#include <sstream>    // file streaming
#include <string>
#include <unordered_map>
#include <vector>

#include <dirent.h>  // may be compatibility issues

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

  SizeType n_data_buffers      = 0;  // number of data points to return when called
  SizeType max_sentences       = 0;  // maximum number of sentences in training set
  SizeType min_sentence_length = 0;  // minimum number of words in a sentence
  SizeType window_size         = 0;  // the size of the context window (one-sided)

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
class TextLoader : public DataLoader<T, typename T::SizeType>
{
public:
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

  TextLoader(TextParams<ArrayType> const &p, SizeType seed = 123456789);

  virtual SizeType               Size() const;
  SizeType                       VocabSize() const;
  virtual bool                   IsDone() const;
  virtual void                   Reset();
  virtual std::pair<T, SizeType> GetAtIndex(SizeType idx);
  virtual std::pair<T, SizeType> GetNext();
  std::pair<T, SizeType>         GetRandom();

  SizeType    VocabLookup(std::string const &word) const;
  std::string VocabLookup(SizeType const idx) const;
  SizeType    GetDiscardCount();

  std::vector<std::pair<std::string, SizeType>> BottomKVocab(SizeType k);
  std::vector<std::pair<std::string, SizeType>> TopKVocab(SizeType k);

  SizeType              GetSentenceIdxFromWordIdx(SizeType word_idx);
  std::vector<SizeType> GetSentenceFromWordIdx(SizeType word_idx);
  SizeType              GetWordOffsetFromWordIdx(SizeType word_idx);

  virtual void                                AddData(std::string const &training_data);
  std::vector<std::pair<std::string, double>> GetKNN(ArrayType const &  embeddings,
                                                     std::string const &word, unsigned int k) const;

protected:
  // training data parsing containers
  SizeType                                               size_ = 0;  // # training pairs
  std::unordered_map<std::string, std::vector<SizeType>> vocab_;  // full unique vocab + frequencies
  std::vector<double>                                    adj_vocab_frequency_;  // word frequencies
  std::vector<std::vector<SizeType>>                     data_;  // all training data by sentence
  std::vector<std::vector<SizeType>>                     discards_;  // record of discarded words
  SizeType discard_sentence_idx_ = 0;  // keeps track of sentences already having discard applied

  // used for iterating through all examples incrementally
  SizeType cursor_;                   // indexes through data
  bool     is_done_         = false;  // tracks progress of cursor
  bool new_random_sequence_ = true;   // whether to generate a new random sequence for training data
  std::vector<SizeType> ran_idx_;     // random indices container

  // params
  TextParams<ArrayType> p_;

  // counts
  SizeType sentence_count_    = 0;  // total sentences in training corpus
  SizeType word_count_        = 0;  // total words in training corpus
  SizeType discard_count_     = 0;  // total count of discarded (frequent) words
  SizeType unique_word_count_ = 1;  // 0 reserved for unknown word

  std::unordered_map<SizeType, SizeType>
      word_idx_sentence_idx;  // lookup table for sentence number from word number
  std::unordered_map<SizeType, SizeType>
      sentence_idx_word_idx;  // lookup table for word number from word sentence

  // containers for the data and labels
  std::vector<std::vector<SizeType>> data_buffers_;

  // random generators
  fetch::random::LaggedFibonacciGenerator<>  lfg_;
  fetch::random::LinearCongruentialGenerator lcg_;

  // should be overwridden when inheriting from text loader
  virtual void     GetData(SizeType idx, ArrayType &ret);
  virtual SizeType GetLabel(SizeType idx);

private:
  void                     ProcessTrainingData(std::string &training_data);
  bool                     CheckValidIndex(SizeType idx);
  std::vector<std::string> StripPunctuationAndLower(std::string &word) const;
  bool                     CheckEndOfSentence(std::string &word);
  std::vector<std::string> GetAllTextFiles(std::string dir_name);
  void GetTextString(std::string const &training_data, std::string &full_training_text);
  void PreProcessWords(std::string &                          training_data,
                       std::vector<std::vector<std::string>> &sentences);
  void BuildVocab(std::vector<std::vector<std::string>> &sentences);
  void DiscardFrequent(std::vector<std::vector<std::string>> &sentences);
  bool DiscardExample(std::string &word);
  std::vector<std::pair<std::string, SizeType>> FindK(SizeType k, bool mode);
};

template <typename T>
TextLoader<T>::TextLoader(TextParams<T> const &p, SizeType seed)
  : cursor_(0)
  , p_(p)
  , lfg_(seed)
  , lcg_(seed)
{
  data_buffers_.resize(p_.n_data_buffers);
}

/**
 *  Returns the total number of words in the training corpus
 */
template <typename T>
typename TextLoader<T>::SizeType TextLoader<T>::Size() const
{
  return word_count_;
}

/**
 * Returns the total number of unique words in the vocabulary
 * @return
 */
template <typename T>
typename TextLoader<T>::SizeType TextLoader<T>::VocabSize() const
{
  return vocab_.size();
}

/**
 * Returns whether we've iterated through all the data
 * @return
 */
template <typename T>
bool TextLoader<T>::IsDone() const
{
  return is_done_;
}

/**
 * resets the cursor for iterating through multiple epochs
 */
template <typename T>
void TextLoader<T>::Reset()
{
  cursor_  = 0;
  is_done_ = false;
}

/**
 * Gets the data at specified index
 * @param idx
 * @return  returns a pair of output buffer and label (i.e. X & Y)
 */
template <typename T>
std::pair<T, typename TextLoader<T>::SizeType> TextLoader<T>::GetAtIndex(SizeType idx)
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
 * get the next data point in order
 * @return
 */
template <typename T>
std::pair<T, typename TextLoader<T>::SizeType> TextLoader<T>::GetNext()
{
  // loop until we find a non-discarded data point
  bool not_found = true;
  while (not_found)
  {
    if (cursor_ >= word_count_)
    {
      Reset();
      is_done_ = true;
    }

    bool valid_idx = CheckValidIndex(cursor_);

    if (valid_idx)
    {
      not_found = false;
    }
    else
    {
      ++cursor_;
    }
  }

  return GetAtIndex(cursor_);
}

/**
 * gets the next data point at random
 * @return
 */
template <typename T>
std::pair<T, typename TextLoader<T>::SizeType> TextLoader<T>::GetRandom()
{
  // loop until we find a non-discarded data point
  bool not_found = true;
  if (ran_idx_.size() == 0)
  {
    new_random_sequence_ = true;
  }

  while (not_found)
  {
    if (cursor_ > vocab_.size())
    {
      Reset();
      is_done_             = true;
      new_random_sequence_ = true;
    }

    if (new_random_sequence_)
    {
      ran_idx_ = std::vector<SizeType>(Size());
      std::iota(ran_idx_.begin(), ran_idx_.end(), 0);
      std::random_shuffle(ran_idx_.begin(), ran_idx_.end());
      new_random_sequence_ = false;
    }

    bool valid_idx = CheckValidIndex(ran_idx_.at(cursor_));

    if (valid_idx)
    {
      not_found = false;
    }
    else
    {
      ++cursor_;
    }
  }

  return GetAtIndex(ran_idx_.at(cursor_));
}

/**
 * checks if a data point may be indexed to create a training pair
 * @return
 */
template <typename T>
bool TextLoader<T>::CheckValidIndex(SizeType idx)
{
  SizeType sentence_idx = GetSentenceIdxFromWordIdx(idx);
  SizeType word_offset  = GetWordOffsetFromWordIdx(idx);
  bool     valid_idx    = true;

  // may only choose indices with a full window either side
  if (p_.full_window)
  {
    bool left_window  = (int(word_offset) - int(p_.window_size) >= 0);
    bool right_window = (word_offset + p_.window_size < data_.at(sentence_idx).size());
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
 * lookup a vocab index for a word
 * @param idx
 * @return
 */
template <typename T>
typename TextLoader<T>::SizeType TextLoader<T>::VocabLookup(std::string const &word) const
{
  assert(vocab_.at(word).at(0) < vocab_.size());
  assert(vocab_.at(word).at(0) != 0);  // dont currently handle unknowns elegantly
  return vocab_.at(word).at(0);
}

/**
 * lookup the string corresponding to a vocab index - EXPENSIVE
 *
 * @param idx
 * @return
 */
template <typename T>
std::string TextLoader<T>::VocabLookup(typename TextLoader<T>::SizeType const idx) const
{
  for (auto &e : vocab_)
  {
    if (e.second.at(0) == idx)
    {
      return e.first;
    }
  }
  return "UNK";
}

/**
 * helper function for getting the K least frequent words
 */
template <typename T>
std::vector<std::pair<std::string, typename TextLoader<T>::SizeType>> TextLoader<T>::BottomKVocab(
    typename TextLoader<T>::SizeType k)
{
  return FindK(k, false);
}

/**
 * helper function for getting the K most frequent words
 */
template <typename T>
std::vector<std::pair<std::string, typename TextLoader<T>::SizeType>> TextLoader<T>::TopKVocab(
    typename TextLoader<T>::SizeType k)
{
  return FindK(k, true);
}

template <typename T>
typename TextLoader<T>::SizeType TextLoader<T>::GetDiscardCount()
{
  return discard_count_;
}

/**
 * return a single sentence from the dataset
 * @param word_idx
 * @return
 */
template <typename T>
typename TextLoader<T>::SizeType TextLoader<T>::GetSentenceIdxFromWordIdx(
    typename TextLoader<T>::SizeType word_idx)
{
  return word_idx_sentence_idx.at(word_idx);
}

/**
 * return a single sentence from the dataset
 * @param word_idx
 * @return
 */
template <typename T>
std::vector<typename TextLoader<T>::SizeType> TextLoader<T>::GetSentenceFromWordIdx(
    typename TextLoader<T>::SizeType word_idx)
{
  SizeType sentence_idx = word_idx_sentence_idx[word_idx];
  return data_[sentence_idx];
}

/**
 * return the word offset within sentence from word idx
 */
template <typename T>
typename TextLoader<T>::SizeType TextLoader<T>::GetWordOffsetFromWordIdx(
    typename TextLoader<T>::SizeType word_idx)
{
  SizeType word_offset;
  SizeType sentence_idx = word_idx_sentence_idx[word_idx];

  if (sentence_idx == 0)
  {
    word_offset = word_idx;
  }
  else
  {
    SizeType cur_word_idx = word_idx - 1;
    SizeType word_idx_for_offset_zero;
    bool     not_found = true;
    while (not_found)
    {
      if (sentence_idx != word_idx_sentence_idx[cur_word_idx])
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

/**
 * Adds text to training data
 * @param training_data
 */
template <typename T>
void TextLoader<T>::AddData(std::string const &training_data)
{
  std::string full_training_text;
  GetTextString(training_data, full_training_text);

  // converts text into training pairs & related preparatory work
  ProcessTrainingData(full_training_text);
}

/**
 * print k nearest neighbours
 * @param vocab
 * @param embeddings
 * @param word
 * @param k
 */
template <typename T>
std::vector<std::pair<std::string, double>> TextLoader<T>::GetKNN(ArrayType const &  embeddings,
                                                                  std::string const &word,
                                                                  unsigned int       k) const
{
  ArrayType wordVector = embeddings.Slice(vocab_.at(word).at(0)).Unsqueeze();

  std::vector<std::pair<SizeType, double>> distances;
  distances.reserve(VocabSize());
  for (SizeType i(1); i < VocabSize(); ++i)  // Start at 1, 0 is UNK
  {
    DataType d = fetch::math::distance::Cosine(wordVector, embeddings.Slice(i).Unsqueeze());
    distances.emplace_back(i, d);
  }
  std::nth_element(distances.begin(), distances.begin() + k, distances.end(),
                   [](std::pair<SizeType, double> const &a, std::pair<SizeType, double> const &b) {
                     return a.second > b.second;
                   });

  std::vector<std::pair<std::string, double>> ret;
  for (SizeType i(0); i < k; ++i)
  {
    ret.emplace_back(std::make_pair(VocabLookup(distances.at(i).first), distances.at(i).second));
  }
  return ret;
}

/**
 * Override this method with a more interesting implementation
 * @param training_data
 */
template <typename T>
void TextLoader<T>::GetData(typename TextLoader<T>::SizeType idx, T &ret)
{
  assert(p_.n_data_buffers == 1);
  SizeType sentence_idx = this->word_idx_sentence_idx.at(idx);
  SizeType word_idx     = this->GetWordOffsetFromWordIdx(idx);
  ret.At(0)             = DataType(this->data_.at(sentence_idx).at(word_idx));
}

template <typename T>
typename TextLoader<T>::SizeType TextLoader<T>::GetLabel(typename TextLoader<T>::SizeType idx)
{
  return 1;
}

/**
 * helper for stripping punctuation from words
 * @param word
 */
template <typename T>
std::vector<std::string> TextLoader<T>::StripPunctuationAndLower(std::string &word) const
{
  std::vector<std::string> ret;

  // replace punct with space and lower case
  SizeType word_idx = 0;
  bool     new_word = true;
  for (auto &c : word)
  {
    if (std::isalpha(c))
    {
      if (new_word)
      {
        ret.emplace_back("");
        ++word_idx;
        new_word = false;
      }
      ret.at(word_idx - 1).push_back((char)std::tolower(c));
    }
    else if (c == '-')
    {
      new_word = true;
    }
    // non-hyphens are assumed to be punctuation that we should ignore
    else
    {
    }
  }

  return ret;
}

/**
 * checks if word contains end of sentence marker
 * @param word a string representing a word with punctuation in tact
 * @return
 */
template <typename T>
bool TextLoader<T>::CheckEndOfSentence(std::string &word)
{
  return std::string(".!?").find(word.back()) != std::string::npos;
}

/**
 * returns a vector of filenames of txt files
 * @param dir_name  the directory to scan
 * @return
 */
template <typename T>
std::vector<std::string> TextLoader<T>::GetAllTextFiles(std::string dir_name)
{
  std::vector<std::string> ret;
  DIR *                    d;
  struct dirent *          ent;
  char *                   p1;
  int                      txt_cmp;
  if ((d = opendir(dir_name.c_str())) != NULL)
  {
    while ((ent = readdir(d)) != NULL)
    {
      strtok(ent->d_name, ".");
      p1 = strtok(NULL, ".");
      if (p1 != NULL)
      {
        txt_cmp = strcmp(p1, "txt");
        if (txt_cmp == 0)
        {
          ret.emplace_back(ent->d_name);
        }
      }
    }
    closedir(d);
  }
  return ret;
}

/**
 * returns the full training text as one string, either gathered from txt files or pass through
 * @param training_data
 * @param full_training_text
 */
template <typename T>
void TextLoader<T>::GetTextString(std::string const &training_data, std::string &full_training_text)
{
  std::vector<std::string> file_names = GetAllTextFiles(training_data);
  if (file_names.size() == 0)
  {
    full_training_text = training_data;
  }
  else
  {
    for (SizeType j = 0; j < file_names.size(); ++j)
    {
      std::string   cur_file = training_data + "/" + file_names[j] + ".txt";
      std::ifstream t(cur_file);

      std::string cur_text((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

      full_training_text += cur_text;
    }
  }
}

/**
 * Preprocesses all training data, then builds vocabulary
 * @param training_data
 */
template <typename T>
void TextLoader<T>::ProcessTrainingData(std::string &training_data)
{
  std::vector<std::vector<std::string>> sentences;

  // strip punctuation and handle casing
  PreProcessWords(training_data, sentences);

  // build unique vocabulary and get word counts
  BuildVocab(sentences);

  // discard words randomly according to word frequency
  DiscardFrequent(sentences);
}

/**
 * removes punctuation and handles casing of all words in training data
 * @param training_data
 */
template <typename T>
void TextLoader<T>::PreProcessWords(std::string &                          training_data,
                                    std::vector<std::vector<std::string>> &sentences)
{
  std::string              word;
  std::vector<std::string> parsed_word{};
  SizeType                 sentence_count = 0;
  bool                     new_sentence   = true;
  for (std::stringstream s(training_data); s >> word;)
  {
    // if new sentence
    if (new_sentence)
    {
      ++sentence_count;

      if (sentence_count_ + sentence_count > p_.max_sentences)
      {
        break;
      }

      sentences.push_back(std::vector<std::string>{});
    }

    // must check this before we strip punctuation
    new_sentence = CheckEndOfSentence(word);

    // strip punctuation & lower case
    parsed_word = StripPunctuationAndLower(word);

    for (auto &cur_word : parsed_word)
    {
      sentences.at(sentence_count - 1).push_back(cur_word);
    }
  }
}

/**
 * builds vocab out of parsed training data
 */
template <typename T>
void TextLoader<T>::BuildVocab(std::vector<std::vector<std::string>> &sentences)
{
  // insert words uniquely into the vocabulary
  vocab_.emplace(std::make_pair("UNK", std::vector<SizeType>({0, 0})));
  for (std::vector<std::string> &cur_sentence : sentences)
  {
    if ((cur_sentence.size() > p_.min_sentence_length) && (sentence_count_ < p_.max_sentences))
    {
      data_.push_back(std::vector<SizeType>({}));
      for (std::string cur_word : cur_sentence)
      {
        // if already seen this word
        SizeType word_idx;
        if (vocab_.find(cur_word) != vocab_.end())
        {
          word_idx           = vocab_[cur_word].at(0);
          SizeType word_freq = vocab_[cur_word].at(1);
          ++word_freq;

          vocab_[cur_word] = std::vector<SizeType>({word_idx, word_freq});
        }
        else
        {
          vocab_.emplace(std::make_pair(cur_word, std::vector<SizeType>({unique_word_count_, 1})));
          word_idx = unique_word_count_;
          unique_word_count_++;
        }
        data_.at(sentence_count_).push_back(word_idx);
        word_idx_sentence_idx.emplace(std::pair<SizeType, SizeType>(word_count_, sentence_count_));
        sentence_idx_word_idx.emplace(std::pair<SizeType, SizeType>(sentence_count_, word_count_));
        word_count_++;
      }
      sentence_count_++;
    }
  }
}

/**
 * discards words in training data set based on word frequency
 */
template <typename T>
void TextLoader<T>::DiscardFrequent(std::vector<std::vector<std::string>> &sentences)
{
  SizeType sentence_count = 0;
  // iterate through all sentences
  for (SizeType sntce_idx = 0; sntce_idx < sentences.size(); sntce_idx++)
  {
    if ((sentences[sntce_idx].size() > p_.min_sentence_length) &&
        (discard_sentence_idx_ + sentence_count < p_.max_sentences))
    {
      discards_.push_back({});
      // iterate through words in the sentence choosing which to discard
      for (SizeType i = 0; i < sentences[sntce_idx].size(); i++)
      {
        if (DiscardExample(sentences[sntce_idx][i]))
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

/**
 * according to mikolov et al. we should throw away examples with proportion to how common the
 * word is
 */
template <typename T>
bool TextLoader<T>::DiscardExample(std::string &word)
{
  // check word actually present
  assert(p_.discard_threshold > 0);
  assert(vocab_.find(word) != vocab_.end());
  SizeType word_frequency = vocab_.at(word).at(1);
  assert(word_frequency > 0);

  double word_probability = double(word_frequency) / double(word_count_);

  double prob_thresh = (std::sqrt(word_probability / p_.discard_threshold) + 1.0);
  prob_thresh *= (p_.discard_threshold / word_probability);
  double f = lfg_.AsDouble();

  if (f < prob_thresh)
  {
    return false;
  }
  return true;
}

/**
 * finds the bottom or top K words by frequency
 * @param k number of words to find
 * @param mode true for topK, false for bottomK
 */
template <typename T>
std::vector<std::pair<std::string, typename TextLoader<T>::SizeType>> TextLoader<T>::FindK(
    SizeType k, bool mode)
{
  std::vector<std::pair<std::string, std::vector<SizeType>>> top_k(k);

  if (mode)
  {
    std::partial_sort_copy(vocab_.begin(), vocab_.end(), top_k.begin(), top_k.end(),
                           [](std::pair<const std::string, std::vector<SizeType>> const &l,
                              std::pair<const std::string, std::vector<SizeType>> const &r) {
                             return l.second.at(1) > r.second.at(1);
                           });
  }
  else
  {
    std::partial_sort_copy(vocab_.begin(), vocab_.end(), top_k.begin(), top_k.end(),
                           [](std::pair<const std::string, std::vector<SizeType>> const &l,
                              std::pair<const std::string, std::vector<SizeType>> const &r) {
                             return l.second.at(1) < r.second.at(1);
                           });
  }

  std::vector<std::pair<std::string, SizeType>> ret(k);
  SizeType                                      tmp = 0;
  for (auto &e : ret)
  {
    e = std::make_pair(top_k.at(tmp).first, top_k.at(tmp).second.at(1));
    ++tmp;
  }

  return ret;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
