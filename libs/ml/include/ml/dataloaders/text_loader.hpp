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

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
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

  SizeType n_data_buffers = 0;  // number of data points to return when called
  SizeType max_sentences  = 0;  // maximum number of sentences in training set

  // optional processing
  bool unigram_table    = false;  // build a unigram table
  bool discard_frequent = false;  // discard frequent words

  // discard params
  double discard_threshold = 0.0;  // random discard probability threshold

  // unigram params
  SizeType unigram_table_size = 0;  // size of unigram table for negative sampling
  double   unigram_power      = 0;  // adjusted unigram distribution
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
  std::vector<std::vector<std::string>>     words_;                // all training data by sentence

  // used for iterating through all examples incrementally
  SizeType cursor_;                   // indexes through data
  bool     is_done_         = false;  // tracks progress of cursor
  bool new_random_sequence_ = true;   // whether to generate a new random sequence for training data
  std::vector<SizeType> ran_idx_;     // random indices container

  // params
  TextParams<ArrayType> p_;

  // counts
  SizeType sentence_count_ = 0;  // total sentences in training corpus
  SizeType word_count_     = 0;  // total words in training corpus
  SizeType discard_count_  = 0;  // total count of discarded (frequent) words

  // containers for the data and labels
  std::vector<std::vector<std::vector<SizeType>>> data_buffers_;
  std::vector<SizeType>                           labels_;

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
   *  Returns the total number of training pairs
   */
  virtual SizeType Size() const
  {
    return size_;
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
        std::make_shared<ArrayType>(std::vector<SizeType>({1, vocab_.size() * p_.n_data_buffers}));

    // pull data from multiple data buffers into single output buffer
    typename ArrayType::Type val;
    for (std::size_t j = 0; j < p_.n_data_buffers; ++j)
    {
      for (SizeType i(0); i < vocab_.size(); ++i)
      {
        val = DataType(data_buffers_.at(j).at(idx).at(i));
        assert((val == DataType(0)) || (val == DataType(1)));
        full_buffer->At((vocab_.size() * j) + i) = val;
      }
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
    if (cursor_ > vocab_.size())
    {
      Reset();
      is_done_ = true;
    }
    else
    {
      is_done_ = false;
    }

    return GetAtIndex(cursor_);
  }

  /**
   * gets the next data point at random
   * @return
   */
  std::pair<std::shared_ptr<T>, SizeType> GetRandom()
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
      is_done_             = false;
    }
    //    return GetAtIndex(SizeType(static_cast<SizeType>(lcg_())) % Size());
    return GetAtIndex(ran_idx_.at(cursor_));
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

private:
  /**
   * method for building up the training pairs that the Get funtions will reference
   * @param training_data
   */
  virtual void BuildTrainingPairs(std::string &training_data) = 0;

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
      //      for (std::size_t j = 0; j < file_names.size(); ++j)
      for (std::size_t j = 0; j < 100; ++j)
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
   * removes punctuation and handles casing of all words in training data
   * @param training_data
   */
  void PreProcessWords(std::string &training_data)
  {
    std::string word;
    words_.push_back(std::vector<std::string>{});
    for (std::stringstream s(training_data); s >> word;)
    {
      // must check this before we strip punctuation
      bool new_sentence = CheckEndOfSentence(word);

      // strip punctuation
      StripPunctuation(word);

      // lower case
      std::transform(word.begin(), word.end(), word.begin(), ::tolower);

      words_[sentence_count_].push_back(word);
      ++word_count_;

      // if new sentence
      if (new_sentence)
      {
        words_.push_back(std::vector<std::string>{});
        ++sentence_count_;

        if (sentence_count_ > p_.max_sentences)
        {
          break;
        }
      }
    }

    // just in case the final word has a full stop or newline - we remove the empty vector
    if (words_.back().size() == 0)
    {
      words_.pop_back();
    }
  }

  /**
   * builds vocab out of parsed training data
   */
  void BuildVocab()
  {
    // insert words uniquely into the vocabulary
    SizeType unique_word_counter = 1;  // 0 reserved for unknown word
    vocab_.insert(std::make_pair("UNK", 0));
    reverse_vocab_.emplace(0, "UNK");
    vocab_frequency_.emplace("UNK", 0);

    for (std::vector<std::string> &cur_sentence : words_)
    {
      for (std::string cur_word : cur_sentence)
      {
        auto ret = vocab_.emplace(cur_word, unique_word_counter);

        if (ret.second)
        {
          reverse_vocab_.emplace(unique_word_counter, cur_word);
          vocab_frequency_[cur_word] = 1;
          ++unique_word_counter;
        }
        else
        {
          vocab_frequency_[cur_word] += 1;
        }
        ++n_words_;
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

        for (std::size_t k = 0; k < n_rows; ++k)
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
   * Preprocesses all training data, then builds vocabulary
   * @param training_data
   */
  void ProcessTrainingData(std::string &training_data)
  {

    // strip punctuation and handle casing
    PreProcessWords(training_data);

    // build unique vocabulary and get word counts
    BuildVocab();

    // build unigram table used for negative sampling
    BuildUnigramTable();

    // discard words randomly according to word frequency
    DiscardFrequent();
  }

  /**
   * discards words in training data set based on word frequency
   */
  void DiscardFrequent()
  {
    if (p_.discard_frequent)
    {
      // iterate through all sentences
      for (SizeType sntce_idx = 0; sntce_idx < sentence_count_; sntce_idx++)
      {
        // iterate through words in the sentence choosing which to discard
        std::vector<SizeType> discards{};
        for (SizeType i = 0; i < words_[sntce_idx].size(); i++)
        {
          if (DiscardExample(words_[sntce_idx][i]))
          {
            discards.push_back(i);
          }
        }

        // reverse iterate and discard those elements
        for (SizeType j = discards.size(); j > 0; --j)
        {
          words_[sntce_idx].erase(words_[sntce_idx].begin() + int(discards[j - SizeType(1)]));
          ++discard_count_;
        }
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
