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

  virtual bool AddData(std::string const &text);

protected:
  // training data parsing containers
  SizeType size_         = 0;  // # training pairs
  SizeType min_sent_len_ = 0;  // minimum length of a permissible sentence
  SizeType max_sent_     = 0;  // maximum number of sentences permissible in vocabulary
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
  std::vector<char> word_break_{'-', '\'', '.', '\t', '\n', '!', '?'};
  std::vector<char> sentence_break_{'.', '\t', '\n', '!', '?'};

  bool                     AddSentenceToVocab(std::vector<std::string> &sentence);
  std::vector<std::string> StripPunctuationAndLower(std::string const &word) const;
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
  // strip case and punctuation in the same way as when vocabulary is added
  std::vector<std::string> parsed_word = StripPunctuationAndLower(word);

  if (parsed_word.empty())
  {
    // word seems to be invalid (e.g. a string of punctuation)
    return std::numeric_limits<SizeType>::max();
  }
  else if (parsed_word.size() > 1)
  {
    // string seems to resolve to multiple words (which would have multiple embeddings)
    // we treat this as an invalid lookup
    return std::numeric_limits<SizeType>::max();
  }
  else
  {
    if (vocab_.find(parsed_word.at(0)) != vocab_.end())
    {
      // return found word
      return vocab_.at(parsed_word.at(0));
    }

    // couldn't find the word in the vocabular
    return std::numeric_limits<SizeType>::max();
  }
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
bool TextLoader<T>::AddData(std::string const &text)
{
  std::string              word;
  std::vector<std::string> parsed_word{};
  bool                     new_sentence   = true;
  bool                     sentence_added = false;
  std::vector<std::string> cur_sentence;
  for (std::stringstream s(text); s >> word;)
  {
    if (sentence_count_ >= max_sent_)
    {
      break;
    }

    if (new_sentence)
    {
      cur_sentence.clear();
    }

    // must check this before we strip punctuation
    new_sentence = CheckEndOfSentence(word);

    // strip punctuation & lower case
    parsed_word = StripPunctuationAndLower(word);

    // sometimes we split a word into two words after removing punctuation
    for (auto &cur_word : parsed_word)
    {
      cur_sentence.emplace_back(cur_word);
    }

    // insert sentence of words uniquely into vocab
    if (new_sentence)
    {
      if (AddSentenceToVocab(cur_sentence))
      {
        sentence_added = true;
        sentence_count_++;
      }
    }
  }

  // if the entire text stream ends without a sentence end character - we'll assume that's the end
  // of a sentence
  if (!new_sentence)
  {
    if (AddSentenceToVocab(cur_sentence))
    {
      sentence_added = true;
      sentence_count_++;
    }
  }

  return sentence_added;
}

///////////////////////////////////////////////
/// Private member function implementations ///
///////////////////////////////////////////////

/**
 * builds vocab out of parsed training data
 */
template <typename T>
bool TextLoader<T>::AddSentenceToVocab(std::vector<std::string> &sentence)
{
  if (sentence.size() >= min_sent_len_)
  {
    data_.push_back(std::vector<SizeType>({}));
    SizeType word_idx;
    for (std::string const &cur_word : sentence)
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
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * helper for stripping punctuation from words
 * @param word
 */
template <typename T>
std::vector<std::string> TextLoader<T>::StripPunctuationAndLower(std::string const &word) const
{
  std::vector<std::string> ret;

  // replace punct with space and lower case
  SizeType word_idx = 0;
  bool     new_word = true;
  for (auto const &c : word)
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
    else if (std::find(word_break_.begin(), word_break_.end(), c) != word_break_.end())
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
  // check last character
  bool end_of_sen = (std::find(sentence_break_.begin(), sentence_break_.end(), word.back()) !=
                     sentence_break_.end());

  // another common way to end the sentence if there is a quote is to use two punctuation chars such
  // as ." !" or ?"
  if (word.back() == '\"')
  {
    if ((std::find(sentence_break_.begin(), sentence_break_.end(), word.end()[-2]) !=
         sentence_break_.end()))
    {
      end_of_sen = true;
    };
  }

  return end_of_sen;
}

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
