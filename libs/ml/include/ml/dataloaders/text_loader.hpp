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

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace ml {
namespace dataloaders {

/**
 * A basic text loader that handles parsing text strings into a vocabulary
 * @tparam T  tensor type
 */
template <typename T>
class TextLoader
{
public:
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

  using WordIdxType   = std::vector<std::vector<SizeType>>;
  using VocabType     = std::unordered_map<std::string, std::vector<SizeType>>;
  using SentencesType = std::vector<std::vector<std::string>>;

  TextLoader() = default;

  SizeType    Size() const;
  SizeType    VocabSize() const;
  SizeType    VocabLookup(std::string const &word) const;
  std::string VocabLookup(SizeType idx) const;

  std::unordered_map<std::string, SizeType> GetVocab() const;

  virtual void AddData(std::string const &text);

protected:
  // training data parsing containers
  SizeType size_         = 0;  // # training pairs
  SizeType min_sent_len_ = 0;  // minimum number of words in permissible sentence
  SizeType max_sent_len_ = 0;  // maximum number of sentences permissible in vocabulary
  std::unordered_map<std::string, SizeType> vocab_;             // unique vocab of words
  std::unordered_map<SizeType, SizeType>    vocab_frequencies;  // the count of each vocab word
  WordIdxType                               data_;  // all training data by sentence_idx[word_idx]

  // counts
  SizeType sentence_count_ = 0;  // total sentences in training corpus
  SizeType word_count_     = 0;  // total words in training corpus

  std::unordered_map<SizeType, SizeType>
      word_idx_sentence_idx;  // lookup table for sentence number from word number
  std::unordered_map<SizeType, SizeType>
      sentence_idx_word_idx;  // lookup table for word number from word sentence

  // should be overwridden when inheriting from text loader
  virtual void     GetData(SizeType idx, ArrayType &ret) = 0;
  virtual SizeType GetLabel(SizeType idx)                = 0;

private:
  void PreProcessWords(std::string const &training_data, SentencesType &sentences);
  void BuildVocab(SentencesType &sentences);
  std::vector<std::string> StripPunctuationAndLower(std::string &word) const;
  bool                     CheckEndOfSentence(std::string &word);
};

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
 *
 * @tparam T Array Type
 * @return unordered map from string to word index & vocab frequency
 */
template <typename T>
std::unordered_map<std::string, typename TextLoader<T>::SizeType> TextLoader<T>::GetVocab() const
{
  return vocab_;
}

/**
 * lookup a vocab index for a word
 * if the word isn't in the vocabulary, returns 0 (index for UNKNOWN word)
 * @param idx
 * @return
 */
template <typename T>
typename TextLoader<T>::SizeType TextLoader<T>::VocabLookup(std::string const &word) const
{
  if (vocab_.find(word) != vocab_.end())
  {
    return vocab_.at(word);
  }
  return SizeType(0);
}

/**
 * lookup the string corresponding to a vocab index
 * if the index doesn't occur in the vocabulary, returns "UNKNOWN"
 * @param idx
 * @return
 */
template <typename T>
std::string TextLoader<T>::VocabLookup(typename TextLoader<T>::SizeType const idx) const
{
  for (auto &e : vocab_)
  {
    if (e.second == idx)
    {
      return e.first;
    }
  }
  return "UNK";
}

/**
 * Adds text to training data
 * @param training_data
 */
template <typename T>
void TextLoader<T>::AddData(std::string const &text)
{
  std::vector<std::vector<std::string>> sentences;

  // strip punctuation and handle casing
  PreProcessWords(text, sentences);

  // build unique vocabulary and get word counts
  BuildVocab(sentences);
}

///////////////////////////////////////////////
/// Private member function implementations ///
///////////////////////////////////////////////

/**
 * removes punctuation and handles casing of all words in training data
 * @param training_data
 */
template <typename T>
void TextLoader<T>::PreProcessWords(std::string const &training_data, SentencesType &sentences)
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

      if (sentence_count_ + sentence_count > max_sent_len_)
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
  vocab_.emplace(std::make_pair("UNK", SizeType(0)));
  vocab_frequencies.emplace(std::make_pair(SizeType(0), SizeType(0)));
  for (std::vector<std::string> &cur_sentence : sentences)
  {
    if ((cur_sentence.size() > min_sent_len_) && (sentence_count_ < max_sent_len_))
    {
      data_.push_back(std::vector<SizeType>({}));
      SizeType word_idx;
      for (std::string cur_word : cur_sentence)
      {
        // if already seen this word
        if (vocab_.find(cur_word) != vocab_.end())
        {
          vocab_frequencies.at(vocab_.at(cur_word))++;
          word_idx = vocab_.at(cur_word);
        }
        else
        {
          word_idx = vocab_.size();
          vocab_frequencies.emplace(std::make_pair(vocab_.size(), SizeType(1)));
          vocab_.emplace(std::make_pair(cur_word, vocab_.size()));
        }
        data_.back().push_back(word_idx);
        word_idx_sentence_idx.emplace(std::pair<SizeType, SizeType>(word_count_, sentence_count_));
        sentence_idx_word_idx.emplace(std::pair<SizeType, SizeType>(sentence_count_, word_count_));
        word_count_++;
      }
      sentence_count_++;
    }
  }
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
    // TODO - catches two character scenarios? like " asdf ?"
    else if ((c == '-') || (c == '\'') || (c == '.') || (c == '\t') || (c == '\n'))
    {
      new_word = true;
    }
    // other punctuation are assumed to be punctuation that we should ignore
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

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
