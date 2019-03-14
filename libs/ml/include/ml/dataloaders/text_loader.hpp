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
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <dirent.h>  // may be compatibility issues

namespace fetch {
namespace ml {

struct TextParams
{
//  SizeType skip_window_        = 0;      // the size of the context window
//  SizeType super_sampling_     = 0;      // # iterations to super sample
//  SizeType k_negative_samples_ = 0;      // # negative samples per positive training pair
//  double   discard_threshold_  = 0.0;    // random discard probability threshold
//  SizeType max_sentences_      = 0;      // maximum number of sentences in training set
//
//  // unigram params
//  SizeType unigram_table_size_ = 10000000;      // size of unigram table for negative sampling
//  std::vector<SizeType> unigram_table_;         // the unigram table container
//  double                unigram_power_ = 0.75;  // adjusted unigram distribution

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

private:

  // training data parsing containers
  SizeType                                  n_words_  = 0;     // # of total words in training data
  std::unordered_map<std::string, SizeType> vocab_;            // full unique vocab
  std::unordered_map<SizeType, std::string> reverse_vocab_;    // full unique vocab
  std::unordered_map<std::string, SizeType> vocab_frequency_;  // word frequencies
  std::vector<double>                       adj_vocab_frequency_;  // word frequencies
  std::vector<std::vector<std::string>>     words_;                // all training data by sentence

  // used for iterating through all examples incrementally
  SizeType cursor_;

  // params
  TextParams p;

  // counts
  SizeType sentence_count_ = 0;  // total sentences in training corpus
  SizeType word_count_     = 0;  // total words in training corpus
  SizeType discard_count_  = 0;  // total count of discarded (frequent) words

  // containers for the data and labels
  std::vector<std::vector<SizeType>> data_input_;
  std::vector<std::vector<SizeType>> data_context_;
  std::vector<SizeType>              labels_;

  // random generators
  fetch::random::LaggedFibonacciGenerator<>  lfg_;
  fetch::random::LinearCongruentialGenerator lcg_;

public:
  TextLoader(std::string &data, TextParams const &params, SizeType seed = 123456789)
    : cursor_(0)
    , skip_window_(skip_window)
    , super_sampling_(super_sampling)
    , k_negative_samples_(k_negative_samples)
    , discard_threshold_(discard_threshold)
    , max_sentences_(max_sentences)
    , lfg_(seed)
    , lcg_(seed)
  {
    assert(skip_window > 0);
    unigram_table_ = std::vector<SizeType>(unigram_table_size_);

    // set up training dataset
    BuildTrainingData(data);
  }

  virtual SizeType Size() const
  {
    return size_;
  }

  SizeType VocabSize()
  {
    return vocab_.size();
  }

  virtual bool IsDone() const
  {
    return cursor_ >= size_;
  }

  virtual void Reset()
  {
    cursor_ = 0;
  }

  virtual std::pair<std::shared_ptr<T>, SizeType> GetAtIndex(SizeType idx)
  {
    std::shared_ptr<T> full_buffer = std::make_shared<ArrayType>(std::vector<SizeType>({1, vocab_.size() * n_output_buffers_}));

    // pull data from multiple data buffers into single output buffer
    typename ArrayType::Type val;
    for (std::size_t j = 0; j < n_output_buffers_; ++j)
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

    return std::make_pair(input_and_context_buffer, label);
  }

  virtual std::pair<std::shared_ptr<T>, SizeType> GetNext()
  {
    return GetAtIndex(cursor_);
  }

  std::pair<std::shared_ptr<T>, SizeType> GetRandom()
  {
    return GetAtIndex(SizeType(static_cast<SizeType>(lcg_())) % Size());
  }

  std::string VocabLookup(SizeType idx)
  {
    return reverse_vocab_[idx];
  }

  SizeType VocabLookup(std::string &idx)
  {
    assert(vocab_[idx] < vocab_.size());
    assert(vocab_[idx] != 0);  // dont currently handle unknowns elegantly
    return vocab_[idx];
  }

private:
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
   * builds all training pairs
   * @param training_data
   */
  void BuildTextString(std::string &training_data)
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

        if (sentence_count_ > max_sentences_)
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

    assert(word_count_ > (skip_window_ * 2));
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
    if (build_unigram_table_)
    {
      // calculate adjusted word frequencies
      double sum_adj_vocab = 0.0;
      for (SizeType j = 0; j < vocab_frequency_.size(); ++j)
      {
        adj_vocab_frequency_.emplace_back(
            std::pow(double(vocab_frequency_[reverse_vocab_[j]]), unigram_power_));
        sum_adj_vocab += adj_vocab_frequency_[j];
      }

      SizeType cur_idx = 0;
      SizeType n_rows  = 0;
      for (SizeType j = 0; j < vocab_.size(); ++j)
      {
        assert(cur_idx < unigram_table_size_);

        double adjusted_word_probability = adj_vocab_frequency_[j] / sum_adj_vocab;

        // word frequency
        n_rows = SizeType(adjusted_word_probability * double(unigram_table_size_));

        for (std::size_t k = 0; k < n_rows; ++k)
        {
          unigram_table_[cur_idx] = j;
          ++cur_idx;
        }
      }
      unigram_table_.resize(cur_idx - 1);
      unigram_table_size_ = cur_idx - 1;
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
   * checks is a context position is valid for the sentence
   * @param target_pos current target position in sentence
   * @param context_pos current context position in sentence
   * @param sentence_len total sentence length
   * @return
   */
  bool WindowPositionCheck(SizeType target_pos, SizeType context_pos, SizeType sentence_len)
  {
    int normalised_context_pos = int(context_pos) - int(skip_window_);
    if (normalised_context_pos == 0)
    {
      // context position is on top of target position
      return false;
    }
    else if (int(target_pos) + normalised_context_pos < 0)
    {
      // context position is before start of sentence
      return false;
    }
    else if (int(target_pos) + normalised_context_pos >= int(sentence_len))
    {
      // context position is after end of sentence
      return false;
    }
    else
    {
      return true;
    }
  }

  /**
   * Dynamic context windows probabilistically reject words from the context window according to
   * unigram distribution There is also a fixed maximum distance
   * @param j the current word position
   * @return
   */
  bool DynamicWindowCheck(SizeType context_position)
  {
    int normalised_context_position = int(context_position) - int(skip_window_);
    int abs_dist_to_target          = fetch::math::Abs(normalised_context_position);
    if (abs_dist_to_target == 1)
    {
      return true;
    }
    else
    {
      double cur_val          = lfg_.AsDouble();
      double accept_threshold = 1.0 / abs_dist_to_target;
      if (cur_val < accept_threshold)
      {
        return true;
      }
      return false;
    }
  }

  /**
   * discards words in training data set based on word frequency
   */
  void DiscardFrequent()
  {
    if (discard_frequent_)
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

    double prob_thresh = (std::sqrt(word_probability / discard_threshold_) + 1.0);
    prob_thresh *= (discard_threshold_ / word_probability);

    //    double prob_thresh      = 1.0 - std::sqrt(discard_threshold_ / word_probability);
    double f = lfg_.AsDouble();

    if (f < prob_thresh)
    {
      return false;
    }
    return true;
  }

  /**
   * calculates the mean frequency of words under unigram for given max skip_window_
   */
  double GetUnigramExpectation()
  {
    double ret = 0;
    for (SizeType i = 0; i < skip_window_; ++i)
    {
      ret += 1.0 / double(i + 1);
    }
    ret *= 2;
    return ret;
  }
};

}  // namespace ml
}  // namespace fetch
