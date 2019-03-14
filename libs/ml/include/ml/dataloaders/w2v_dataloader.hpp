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

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class W2VLoader : public DataLoader<std::shared_ptr<T>, typename T::SizeType>
{
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

private:
  // training data parsing containers
  SizeType                                  size_     = 0;     // # training pairs
  SizeType                                  pos_size_ = 0;     // # positive training pairs
  SizeType                                  neg_size_ = 0;     // # negative training pairs
  SizeType                                  n_words_  = 0;     // # of total words in training data
  std::unordered_map<std::string, SizeType> vocab_;            // full unique vocab
  std::unordered_map<SizeType, std::string> reverse_vocab_;    // full unique vocab
  std::unordered_map<std::string, SizeType> vocab_frequency_;  // word frequencies
  std::vector<double>                       adj_vocab_frequency_;  // word frequencies
  std::vector<std::vector<std::string>>     words_;                // all training data by sentence

  // used for iterating through all examples incrementally
  SizeType cursor_;

  bool     cbow_               = false;  // CBOW or SGNS model
  SizeType skip_window_        = 0;      // the size of the context window
  SizeType super_sampling_     = 0;      // # iterations to super sample
  SizeType k_negative_samples_ = 0;      // # negative samples per positive training pair
  double   discard_threshold_  = 0.0;    // random discard probability threshold
  SizeType max_sentences_      = 0;      // maximum number of sentences in training set

  SizeType sentence_count_ = 0;  // total sentences in training corpus
  SizeType word_count_     = 0;  // total words in training corpus
  SizeType discard_count_  = 0;  // total count of discarded (frequent) words

  SizeType unigram_table_size_ = 10000000;      // size of unigram table for negative sampling
  std::vector<SizeType> unigram_table_;         // the unigram table container
  double                unigram_power_ = 0.75;  // adjusted unigram distribution

  // containers for the data and labels
  std::vector<std::vector<SizeType>> data_input_;
  std::vector<std::vector<SizeType>> data_context_;
  std::vector<SizeType>              labels_;

  // random generators
  fetch::random::LaggedFibonacciGenerator<>  lfg_;
  fetch::random::LinearCongruentialGenerator lcg_;

public:
  W2VLoader(std::string &data, bool cbow, SizeType skip_window, SizeType super_sampling,
            SizeType k_negative_samples, double discard_threshold, SizeType max_sentences,
            SizeType seed = 123456789)
    : cursor_(0)
    , cbow_(cbow)
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
    std::shared_ptr<T> input_and_context_buffer =
        std::make_shared<ArrayType>(std::vector<SizeType>({1, vocab_.size() * 2}));

    // input word
    typename ArrayType::Type val;
    for (SizeType i(0); i < vocab_.size(); ++i)
    {
      val = DataType(data_input_[idx][i]);
      assert((val == DataType(0)) || (val == DataType(1)));
      input_and_context_buffer->At(i) = val;
    }

    // context word
    for (SizeType i(0); i < vocab_.size(); ++i)
    {
      val = typename ArrayType::Type(data_context_[idx][i]);
      assert((val == DataType(0)) || (val == DataType(1)));
      input_and_context_buffer->At(vocab_.size() + i) =
          typename ArrayType::Type(data_context_[idx][i]);
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
  void BuildTrainingData(std::string &training_data)
  {
    std::cout << "loading training data: " << std::endl;
    std::string full_training_text;
    GetTextString(training_data, full_training_text);

    if (cbow_)
    {
      throw std::runtime_error("cbow_ not yet implemented");
    }
    else
    {
      std::cout << "preprocessing text: " << std::endl;
      // convers text into training pairs & related preparatory work
      ProcessTrainingData(full_training_text);

      std::cout << "generating positive and negative training pairs: " << std::endl;
      for (std::size_t j = 0; j < super_sampling_; ++j)
      {
        // generate positive examples - i.e. within context window
        GeneratePositive();

        // generate negative training examples - i.e. from outside of training window
        GenerateNegative();
      }
    }
  }

  /**
   * generates positive training pairs
   */
  void GeneratePositive()
  {
    SizeType              cur_vocab_idx   = 0;
    SizeType              cur_context_idx = 0;
    std::vector<SizeType> input_one_hot(vocab_.size());
    std::vector<SizeType> context_one_hot(vocab_.size());
    SizeType              one_hot_tmp;

    // iterate through all sentences
    for (SizeType sntce_idx = 0; sntce_idx < sentence_count_; sntce_idx++)
    {
      for (SizeType i = 0; i < words_[sntce_idx].size(); i++)
      {
        // current input word idx
        cur_vocab_idx = SizeType(vocab_[words_[sntce_idx][i]]);
        assert(cur_vocab_idx > 0);  // unknown words not yet handled
        assert(cur_vocab_idx < vocab_.size());

        for (SizeType j = 0; j < (2 * skip_window_) + 1; j++)
        {
          if (WindowPositionCheck(i, j, words_[sntce_idx].size()) && DynamicWindowCheck(j))
          {
            std::string context_word = words_[sntce_idx][i + j - skip_window_];

            // context word idx
            cur_context_idx = vocab_[context_word];
            assert(cur_context_idx > 0);  // unknown words not yet handled
            assert(cur_context_idx < vocab_.size());

            // build input one hot
            for (SizeType k = 0; k < vocab_.size(); k++)
            {
              one_hot_tmp = SizeType(k == cur_vocab_idx);
              assert((one_hot_tmp == 0) || (one_hot_tmp == 1));
              input_one_hot[k] = one_hot_tmp;
            }

            // build context one hot
            for (SizeType k = 0; k < vocab_.size(); k++)
            {
              one_hot_tmp = SizeType(k == cur_context_idx);
              assert((one_hot_tmp == 0) || (one_hot_tmp == 1));
              context_one_hot[k] = one_hot_tmp;
            }

            // assign data
            data_input_.emplace_back(input_one_hot);
            data_context_.emplace_back(context_one_hot);

            // assign label
            labels_.emplace_back(SizeType(1));

            ++size_;
            ++pos_size_;
          }
        }
      }
    }
  }

  /**
   * generates negative training pairs
   */
  void GenerateNegative()
  {
    SizeType              cur_vocab_idx = 0;
    std::vector<SizeType> input_one_hot(vocab_.size());
    std::vector<SizeType> context_one_hot(vocab_.size());
    SizeType              one_hot_tmp;
    SizeType              n_negative_training_pairs_per_word =
        static_cast<SizeType>(GetUnigramExpectation() * double(k_negative_samples_));
    SizeType negative_context_idx;

    // iterate through all sentences
    for (SizeType sntce_idx = 0; sntce_idx < sentence_count_; sntce_idx++)
    {
      for (SizeType i = 0; i < words_[sntce_idx].size(); i++)
      {
        // current input word idx
        cur_vocab_idx = SizeType(vocab_[words_[sntce_idx][i]]);
        assert(cur_vocab_idx > 0);  // unknown words not yet handled
        assert(cur_vocab_idx < vocab_.size());

        for (SizeType j = 0; j < n_negative_training_pairs_per_word; j++)
        {
          // randomly select a negative context word
          bool ongoing = true;
          while (ongoing)
          {
            // randomly select a word from the unigram_table
            SizeType ran_val     = SizeType(lcg_());
            SizeType max_val     = unigram_table_size_;
            negative_context_idx = unigram_table_[ran_val % max_val];
            assert(negative_context_idx > 0);
            assert(negative_context_idx < vocab_.size());
            ongoing = false;

            // check if selected negative word actually within context window
            for (SizeType j = 0; j < (2 * skip_window_) + 1; j++)
            {
              if (WindowPositionCheck(i, j, words_[sntce_idx].size()))
              {
                if (words_[sntce_idx][j] == reverse_vocab_[negative_context_idx])
                {
                  ongoing = true;
                }
              }
            }
          }

          // build input one hot
          for (unsigned int k = 0; k < vocab_.size(); k++)
          {
            one_hot_tmp = SizeType(k == cur_vocab_idx);
            assert((one_hot_tmp == 0) || (one_hot_tmp == 1));
            input_one_hot[k] = one_hot_tmp;
          }

          // build context one hot
          for (unsigned int k = 0; k < vocab_.size(); k++)
          {
            one_hot_tmp = SizeType(k == negative_context_idx);
            assert((one_hot_tmp == 0) || (one_hot_tmp == 1));
            context_one_hot[k] = one_hot_tmp;
          }

          data_input_.push_back(input_one_hot);
          data_context_.push_back(context_one_hot);

          // assign label
          labels_.push_back(SizeType(0));

          ++size_;
          ++neg_size_;
        }
      }
    }
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
