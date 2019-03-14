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
#include "ml/dataloaders/text_loader.hpp"

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
namespace dataloaders {

/**
 * additional params only relent for Skipgram models
 */
template <typename T>
struct SkipGramTextParams : TextParams<T>
{
public:
  SkipGramTextParams()
    : TextParams<T>(){};

  using SizeType              = typename T::SizeType;
  SizeType skip_window        = 0;  // the size of the context window
  SizeType super_sampling     = 0;  // # iterations to super sample
  SizeType k_negative_samples = 0;  // # negative samples per positive training pair
};

/**
 * A custom dataloader for the Word2Vec example
 * @tparam T  tensor type
 */
template <typename T>
class SkipGramLoader : public TextLoader<T>
{
  using ArrayType = T;
  using DataType  = typename T::Type;
  using SizeType  = typename T::SizeType;

private:
  // training data parsing containers
  SizeType pos_size_ = 0;  // # positive training pairs
  SizeType neg_size_ = 0;  // # negative training pairs

  SkipGramTextParams<T> p_;

public:
  SkipGramLoader(std::string &data, SkipGramTextParams<T> p, SizeType seed = 123456789)
    : TextLoader<T>(data, p, seed)
    , p_(p)
  {

    // sanity checks on SkipGram parameters
    assert(this->word_count_ > (p_.skip_window * 2));
    assert(p_.skip_window > 0);

    // set up training dataset
    BuildTrainingPairs(data);
  }

private:
  /**
   * builds all training pairs
   * @param training_data
   */
  void BuildTrainingPairs(std::string &training_data)
  {
    std::cout << "generating positive and negative training pairs: " << std::endl;
    for (std::size_t j = 0; j < p_.super_sampling; ++j)
    {
      // generate positive examples - i.e. within context window
      GeneratePositive();

      // generate negative training examples - i.e. from outside of training window
      GenerateNegative();
    }
  }

  /**
   * generates positive training pairs
   */
  void GeneratePositive()
  {
    SizeType              cur_vocab_idx   = 0;
    SizeType              cur_context_idx = 0;
    std::vector<SizeType> input_one_hot(this->vocab_.size());
    std::vector<SizeType> context_one_hot(this->vocab_.size());
    SizeType              one_hot_tmp;

    // iterate through all sentences
    for (SizeType sntce_idx = 0; sntce_idx < this->sentence_count_; sntce_idx++)
    {
      for (SizeType i = 0; i < this->words_[sntce_idx].size(); i++)
      {
        // current input word idx
        cur_vocab_idx = SizeType(this->vocab_[this->words_[sntce_idx][i]]);
        assert(cur_vocab_idx > 0);  // unknown words not yet handled
        assert(cur_vocab_idx < this->vocab_.size());

        for (SizeType j = 0; j < (2 * p_.skip_window) + 1; j++)
        {
          if (WindowPositionCheck(i, j, this->words_[sntce_idx].size()) && DynamicWindowCheck(j))
          {
            std::string context_word = this->words_[sntce_idx][i + j - p_.skip_window];

            // context word idx
            cur_context_idx = this->vocab_[context_word];
            assert(cur_context_idx > 0);  // unknown words not yet handled
            assert(cur_context_idx < this->vocab_.size());

            // build input one hot
            for (SizeType k = 0; k < this->vocab_.size(); k++)
            {
              one_hot_tmp = SizeType(k == cur_vocab_idx);
              assert((one_hot_tmp == 0) || (one_hot_tmp == 1));
              input_one_hot[k] = one_hot_tmp;
            }

            // build context one hot
            for (SizeType k = 0; k < this->vocab_.size(); k++)
            {
              one_hot_tmp = SizeType(k == cur_context_idx);
              assert((one_hot_tmp == 0) || (one_hot_tmp == 1));
              context_one_hot[k] = one_hot_tmp;
            }

            // assign data
            this->data_buffers_.at(0).emplace_back(input_one_hot);
            this->data_buffers_.at(1).emplace_back(context_one_hot);

            // assign label
            this->labels_.emplace_back(SizeType(1));

            ++this->size_;
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
    std::vector<SizeType> input_one_hot(this->vocab_.size());
    std::vector<SizeType> context_one_hot(this->vocab_.size());
    SizeType              one_hot_tmp;
    SizeType              n_negative_training_pairs_per_word =
        static_cast<SizeType>(this->GetUnigramExpectation() * double(p_.k_negative_samples));
    SizeType negative_context_idx;

    // iterate through all sentences
    for (SizeType sntce_idx = 0; sntce_idx < this->sentence_count_; sntce_idx++)
    {
      for (SizeType i = 0; i < this->words_[sntce_idx].size(); i++)
      {
        // current input word idx
        cur_vocab_idx = SizeType(this->vocab_[this->words_[sntce_idx][i]]);
        assert(cur_vocab_idx > 0);  // unknown words not yet handled
        assert(cur_vocab_idx < this->vocab_.size());

        for (SizeType j = 0; j < n_negative_training_pairs_per_word; j++)
        {
          // randomly select a negative context word
          bool ongoing = true;
          while (ongoing)
          {
            // randomly select a word from the unigram_table
            SizeType ran_val     = SizeType(this->lcg_());
            SizeType max_val     = p_.unigram_table_size;
            negative_context_idx = this->unigram_table_[ran_val % max_val];
            assert(negative_context_idx > 0);
            assert(negative_context_idx < this->vocab_.size());
            ongoing = false;

            // check if selected negative word actually within context window
            for (SizeType j = 0; j < (2 * p_.skip_window) + 1; j++)
            {
              if (WindowPositionCheck(i, j, this->words_[sntce_idx].size()))
              {
                if (this->words_[sntce_idx][j] == this->reverse_vocab_[negative_context_idx])
                {
                  ongoing = true;
                }
              }
            }
          }

          // build input one hot
          for (unsigned int k = 0; k < this->vocab_.size(); k++)
          {
            one_hot_tmp = SizeType(k == cur_vocab_idx);
            assert((one_hot_tmp == 0) || (one_hot_tmp == 1));
            input_one_hot[k] = one_hot_tmp;
          }

          // build context one hot
          for (unsigned int k = 0; k < this->vocab_.size(); k++)
          {
            one_hot_tmp = SizeType(k == negative_context_idx);
            assert((one_hot_tmp == 0) || (one_hot_tmp == 1));
            context_one_hot[k] = one_hot_tmp;
          }

          this->data_buffers_.at(0).push_back(input_one_hot);
          this->data_buffers_.at(1).push_back(context_one_hot);

          // assign label
          this->labels_.push_back(SizeType(0));

          ++this->size_;
          ++neg_size_;
        }
      }
    }
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
    int normalised_context_pos = int(context_pos) - int(p_.skip_window);
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
    int normalised_context_position = int(context_position) - int(p_.skip_window);
    int abs_dist_to_target          = fetch::math::Abs(normalised_context_position);
    if (abs_dist_to_target == 1)
    {
      return true;
    }
    else
    {
      double cur_val          = this->lfg_.AsDouble();
      double accept_threshold = 1.0 / abs_dist_to_target;
      if (cur_val < accept_threshold)
      {
        return true;
      }
      return false;
    }
  }

  /**
   * calculates the mean frequency of words under unigram for given max skip_window
   */
  double GetUnigramExpectation()
  {
    double ret = 0;
    for (SizeType i = 0; i < p_.skip_window; ++i)
    {
      ret += 1.0 / double(i + 1);
    }
    ret *= 2;
    return ret;
  }
};

}  // namespace dataloaders
}  // namespace ml
}  // namespace fetch
