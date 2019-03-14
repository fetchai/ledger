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
#include "ml/dataloaders/dataloader.hpp"

#include <algorithm>                // random_shuffle
#include <fstream>                  // file streaming
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
  using SizeType = typename T::SizeType;

public:
  TextParams(){};

  SizeType n_data_buffers      = 0;  // number of data points to return when called
  SizeType max_sentences       = 0;  // maximum number of sentences in training set
  SizeType min_sentence_length = 0;  // minimum number of words in a sentence
  SizeType window_size         = 0;  // the size of the context window

  // optional processing
  bool unigram_table    = false;  // build a unigram table
  bool discard_frequent = false;  // discard frequent words

  // discard params
  double discard_threshold = 0.0;  // random discard probability threshold

  // unigram params
  SizeType unigram_table_size = 10000000;  // size of unigram table for negative sampling
  double   unigram_power      = 0.75;      // adjusted unigram distribution
  SizeType unigram_precision  = 10;
};

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class TextLoader : public DataLoader<std::shared_ptr<T>, typename T::SizeType>
{
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

protected:
  // training data parsing containers
  SizeType                                  size_    = 0;      // # training pairs
  SizeType                                  n_words_ = 0;      // # of total words in training data
  std::unordered_map<std::string, SizeType> vocab_;            // full unique vocab
  std::unordered_map<SizeType, std::string> reverse_vocab_;    // full unique vocab
  std::unordered_map<std::string, SizeType> vocab_frequency_;  // word frequencies
  std::vector<double>                       adj_vocab_frequency_;  // word frequencies
  std::vector<std::vector<SizeType>>        data_;                 // all training data by sentence
  std::vector<std::vector<SizeType>>        discards_;             // record of discarded words
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
  std::vector<SizeType>              labels_;

  // random generators
  fetch::random::LaggedFibonacciGenerator<>  lfg_;
  fetch::random::LinearCongruentialGenerator lcg_;

  // the unigram table container
  std::vector<SizeType> unigram_table_;

public:
  TextLoader(std::string &data, TextParams<ArrayType> const &p, SizeType seed = 123456789)
    : cursor_(0)
    , p_(p)
    , lfg_(seed)
    , lcg_(seed)
  {

    if (p_.unigram_table)
    {
      unigram_table_ = std::vector<SizeType>(p_.unigram_table_size);
    }

    data_buffers_.resize(p_.n_data_buffers);

    // set up training dataset
    BuildText(data);
  }

  /**
   *  Returns the total number of words
   */
  virtual SizeType Size() const
  {
    return word_count_;
  }

  /**
   * Returns the total number of unique words in the vocabulary
   * @return
   */
  SizeType VocabSize()
  {
    return vocab_.size();
  }

  /**
   * Returns whether we've iterated through all the data
   * @return
   */
  virtual bool IsDone() const
  {
    return cursor_ >= size_;
  }

  /**
   * resets the cursor for iterating through multiple epochs
   */
  virtual void Reset()
  {
    cursor_ = 0;
  }

  /**
   * Gets the data at specified index
   * @param idx
   * @return  returns a pair of output buffer and label (i.e. X & Y)
   */
  virtual std::pair<std::shared_ptr<T>, SizeType> GetAtIndex(SizeType idx)
  {
    std::shared_ptr<T> full_buffer =
        std::make_shared<ArrayType>(std::vector<SizeType>({1, p_.n_data_buffers}));

    // pull data from multiple data buffers into single output buffer
    std::vector<SizeType> buffer_idxs = GetData(idx);
    assert(buffer_idxs.size() == p_.n_data_buffers);

    for (SizeType j = 0; j < p_.n_data_buffers; ++j)
    {
      SizeType sentence_idx = word_idx_sentence_idx[idx];
      SizeType word_idx     = GetWordOffsetFromWordIdx(idx);
      full_buffer->At(j)    = DataType(data_.at(sentence_idx).at(word_idx));
    }

    SizeType label = SizeType(labels_[idx]);
    cursor_++;

    return std::make_pair(full_buffer, label);
  }

  /**
   * get the next data point in order
   * @return
   */
  virtual std::pair<std::shared_ptr<T>, SizeType> GetNext()
  {
    // loop until we find a non-discarded data point
    bool not_found = true;
    while(not_found)
    {
      if (cursor_ > vocab_.size())
      {
        Reset();
        is_done_ = true;
      }

      SizeType sentence_idx = GetSentenceIdxFromWordIdx(cursor_);
      SizeType word_offset = GetWordOffsetFromWordIdx(cursor_);
      if (!(discards_.at(sentence_idx).at(word_offset)))
      {
        is_done_ = false;
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
  std::pair<std::shared_ptr<T>, SizeType> GetRandom()
  {
    // loop until we find a non-discarded data point
    bool not_found = true;
    while(not_found)
    {
      if (cursor_ > vocab_.size())
      {
        Reset();
        is_done_ = true;
        new_random_sequence_ = true;
      }

      SizeType sentence_idx = GetSentenceIdxFromWordIdx(cursor_);
      SizeType word_offset = GetWordOffsetFromWordIdx(cursor_);
      if (!(discards_.at(sentence_idx).at(word_offset)))
      {
        is_done_ = false;
        not_found = false;
      }
      else
      {
        ++cursor_;
      }
    }

    if (new_random_sequence_)
    {
      ran_idx_ = std::vector<SizeType>(Size());
      std::iota(ran_idx_.begin(), ran_idx_.end(), 0);
      std::random_shuffle(ran_idx_.begin(), ran_idx_.end());
      new_random_sequence_ = false;
      is_done_             = false;
    }
    //    return GetAtIndex(SizeType(static_cast<SizeType>(lcg_())) % Size());
    return GetAtIndex(ran_idx_.at(cursor_));
  }

  /**
   * add more text data after construction
   * @param s
   * @return
   */
  bool AddData(std::string const &s)
  {
    std::vector<SizeType> indexes = BuildText(s);
    if (indexes.size() >= 2 * p_.window_size + 1)
    {
      data_.push_back(std::move(indexes));
      return true;
    }
    return false;
  }

  /**
   * lookup a word given the vocab index
   * @param idx
   * @return
   */
  std::string VocabLookup(SizeType idx)
  {
    return reverse_vocab_[idx];
  }

  /**
   * lookup a vocab index for a word
   * @param idx
   * @return
   */
  SizeType VocabLookup(std::string &idx)
  {
    assert(vocab_[idx] < vocab_.size());
    assert(vocab_[idx] != 0);  // dont currently handle unknowns elegantly
    return vocab_[idx];
  }

  /**
   * return a single sentence from the dataset
   * @param word_idx
   * @return
   */
  SizeType GetSentenceIdxFromWordIdx(SizeType word_idx)
  {
    return word_idx_sentence_idx[word_idx];
  }

  /**
   * return a single sentence from the dataset
   * @param word_idx
   * @return
   */
  std::vector<SizeType> GetSentenceFromWordIdx(SizeType word_idx)
  {
    SizeType sentence_idx = word_idx_sentence_idx[word_idx];
    return data_[sentence_idx];
  }

  /**
   * return the word offset within sentence from word idx
   */
  SizeType GetWordOffsetFromWordIdx(SizeType word_idx)
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

          word_offset = word_idx - word_idx_for_offset_zero;
        }
      }
      not_found = false;
    }
    return word_offset;
  }

private:
  /**
   * method for building up the training pairs that the Get funtions will reference
   * @param training_data
   */
  virtual std::vector<SizeType> GetData(SizeType idxs) = 0;

  /**
   * helper for stripping punctuation from words
   * @param word
   */
  void StripPunctuation(std::string &word)
  {
    std::string result;
    std::remove_copy_if(word.begin(), word.end(),
                        std::back_inserter(result),  // Store output
                        std::ptr_fun<int, int>(&std::ispunct));
    word = result;
  }

  /**
   * checks if word contains end of sentence marker
   * @param word a string representing a word with punctuation in tact
   * @return
   */
  bool CheckEndOfSentence(std::string &word)
  {
    // if new sentence
    if ((std::string(1, word.back()) == std::string(".")) ||
        (std::string(1, word.back()) == std::string("!")) ||
        (std::string(1, word.back()) == std::string("?")))
    {
      return true;
    }
    return false;
  }

  /**
   * returns a vector of filenames of txt files
   * @param dir_name  the directory to scan
   * @return
   */
  std::vector<std::string> GetAllTextFiles(std::string dir_name)
  {
    std::vector<std::string> ret;
    DIR *                    d;
    struct dirent *          ent;
    char *                   p1;
    char *                   p2;
    int                      txt_cmp;
    if ((d = opendir(dir_name.c_str())) != NULL)
    {
      while ((ent = readdir(d)) != NULL)
      {
        p1 = strtok(ent->d_name, ".");
        p2 = strtok(NULL, ".");
        if (p2 != NULL)
        {
          txt_cmp = strcmp(p2, "txt");
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
  void GetTextString(std::string &training_data, std::string &full_training_text)
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
   * builds all training text
   * @param training_data
   */
  void BuildText(std::string &training_data)
  {
    std::string full_training_text;
    GetTextString(training_data, full_training_text);

    // converts text into training pairs & related preparatory work
    ProcessTrainingData(full_training_text);
  }

  /**
   * Preprocesses all training data, then builds vocabulary
   * @param training_data
   */
  void ProcessTrainingData(std::string &training_data)
  {
    std::vector<std::vector<std::string>> sentences;

    // strip punctuation and handle casing
    PreProcessWords(training_data, sentences);

    // build unique vocabulary and get word counts
    BuildVocab(sentences);

    // build unigram table used for negative sampling
    BuildUnigramTable();

    // discard words randomly according to word frequency
    DiscardFrequent(sentences);
  }

  /**
   * removes punctuation and handles casing of all words in training data
   * @param training_data
   */
  void PreProcessWords(std::string &training_data, std::vector<std::vector<std::string>> &sentences)
  {
    std::string word;
    sentences.push_back(std::vector<std::string>{});
    SizeType word_offset = 0;
    for (std::stringstream s(training_data); s >> word;)
    {
      // must check this before we strip punctuation
      bool new_sentence = CheckEndOfSentence(word);

      // strip punctuation
      StripPunctuation(word);

      // lower case
      std::transform(word.begin(), word.end(), word.begin(), ::tolower);

      sentences[sentence_count_].push_back(word);
      ++word_count_;

      ++word_offset;

      // if new sentence
      if (new_sentence)
      {
        sentences.push_back(std::vector<std::string>{});
        ++sentence_count_;
        word_offset = 0;

        if (sentence_count_ > p_.max_sentences)
        {
          break;
        }
      }
    }

    // just in case the final word has a full stop or newline - we remove the empty vector
    if (sentences.back().size() == 0)
    {
      sentences.pop_back();
    }
  }

  /**
   * builds vocab out of parsed training data
   */
  void BuildVocab(std::vector<std::vector<std::string>> &sentences)
  {
    // insert words uniquely into the vocabulary
    vocab_.emplace(std::make_pair("UNK", 0));
    reverse_vocab_.emplace(0, "UNK");
    vocab_frequency_.emplace("UNK", 0);

    sentence_count_ = 0;
    word_count_     = 0;
    SizeType cur_val;
    for (std::vector<std::string> &cur_sentence : sentences)
    {
      data_.emplace_back(std::vector<SizeType>({}));
      if (cur_sentence.size() > p_.min_sentence_length)
      {
        for (std::string cur_word : cur_sentence)
        {
          auto ret = vocab_.emplace(cur_word, unique_word_count_);

          if (ret.second)
          {
            reverse_vocab_.emplace(unique_word_count_, cur_word);
            vocab_frequency_[cur_word] = 1;
            ++unique_word_count_;
          }
          else
          {
            vocab_frequency_[cur_word] += 1;
          }
          ++n_words_;

          cur_val = (*ret.first).second;
          data_.at(sentence_count_).emplace_back(cur_val);
          word_idx_sentence_idx.emplace(
              std::pair<SizeType, SizeType>(word_count_, sentence_count_));
          sentence_idx_word_idx.emplace(
              std::pair<SizeType, SizeType>(sentence_count_, word_count_));
          word_count_++;
        }
        sentence_count_++;
      }
    }
  }

  /**
   * builds the unigram table for negative sampling
   */
  void BuildUnigramTable()
  {
    if (p_.unigram_table)
    {
      // calculate adjusted word frequencies
      double sum_adj_vocab = 0.0;
      for (SizeType j = 0; j < vocab_frequency_.size(); ++j)
      {
        adj_vocab_frequency_.emplace_back(
            std::pow(double(vocab_frequency_[reverse_vocab_[j]]), p_.unigram_power));
        sum_adj_vocab += adj_vocab_frequency_[j];
      }

      SizeType cur_idx = 0;
      SizeType n_rows  = 0;
      for (SizeType j = 0; j < vocab_.size(); ++j)
      {
        assert(cur_idx < p_.unigram_table_size);

        double adjusted_word_probability = adj_vocab_frequency_[j] / sum_adj_vocab;

        // word frequency
        n_rows = SizeType(adjusted_word_probability * double(p_.unigram_table_size));

        for (SizeType k = 0; k < n_rows; ++k)
        {
          unigram_table_[cur_idx] = j;
          ++cur_idx;
        }
      }
      unigram_table_.resize(cur_idx - 1);
      p_.unigram_table_size = cur_idx - 1;
    }
  }

  /**
   * discards words in training data set based on word frequency
   */
  void DiscardFrequent(std::vector<std::vector<std::string>> &sentences)
  {
    if (p_.discard_frequent)
    {
      // iterate through all sentences
      for (SizeType sntce_idx = 0; sntce_idx < sentences.size(); sntce_idx++)
      {
        discards_.push_back({});
        // iterate through words in the sentence choosing which to discard
        for (SizeType i = 0; i < sentences[sntce_idx].size(); i++)
        {
          if (DiscardExample(sentences[sntce_idx][i]))
          {
            discards_.at(discard_sentence_idx_).push_back(1);
            ++discard_count_;
          }
          else
          {
            discards_.at(discard_sentence_idx_).push_back(0);
          }
        }

        ++discard_sentence_idx_;
      }
    }
  }

  /**
   * according to mikolov et al. we should throw away examples with proportion to how common the
   * word is
   */
  bool DiscardExample(std::string &word)
  {
    double word_probability = double(vocab_frequency_[word]) / double(n_words_);

    double prob_thresh = (std::sqrt(word_probability / p_.discard_threshold) + 1.0);
    prob_thresh *= (p_.discard_threshold / word_probability);
    double f = lfg_.AsDouble();

    if (f < prob_thresh)
    {
      return false;
    }
    return true;
  }
};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
