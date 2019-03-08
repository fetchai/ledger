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

#include "ml/dataloaders/dataloader.hpp"

#include <algorithm>
#include <exception>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

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
  using SizeType  = typename T::SizeType;

public:
  W2VLoader(std::string &data, SizeType skip_window, bool cbow, SizeType k_negative_samples,
            double discard_threshold)
    : cursor_(0)
    , skip_window_(skip_window)
    , cbow_(cbow)
    , k_negative_samples_(k_negative_samples)
    , discard_threshold_(discard_threshold)
  {
    assert(skip_window > 0);

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

  virtual std::pair<std::pair<std::shared_ptr<T>, std::shared_ptr<T>>, SizeType> GetAtIndex(
      SizeType idx)
  {
    std::shared_ptr<T> input_buffer =
        std::make_shared<ArrayType>(std::vector<SizeType>({1, vocab_.size()}));
    std::shared_ptr<T> context_buffer =
        std::make_shared<ArrayType>(std::vector<SizeType>({1, vocab_.size()}));

    // input word
    typename ArrayType::Type val;
    for (SizeType i(0); i < vocab_.size(); ++i)
    {
      val = typename ArrayType::Type(data_input_[idx][i]);
      assert((val == 0) || (val == 1));
      input_buffer->At(i) = val;
    }

    // context word
    for (SizeType i(0); i < vocab_.size(); ++i)
    {
      val = typename ArrayType::Type(data_context_[idx][i]);
      assert((val == 0) || (val == 1));
      context_buffer->At(i) = typename ArrayType::Type(data_context_[idx][i]);
    }

    SizeType label = SizeType(labels_[idx]);
    cursor_++;

    return std::make_pair(std::make_pair(input_buffer, context_buffer), label);
  }

  virtual std::pair<std::shared_ptr<T>, SizeType> GetNext()
  {
    throw std::runtime_error(
        "GetNext temporariliy disabled since does"
        "not conform to DataLoader abstract class standard");
    std::pair<std::pair<std::shared_ptr<T>, std::shared_ptr<T>>, SizeType> tmp =
        GetAtIndex(cursor_++);
    return std::make_pair(tmp.first.first, tmp.second);
  }

  std::pair<std::pair<std::shared_ptr<T>, std::shared_ptr<T>>, SizeType> GetRandom()
  {
    return GetAtIndex(static_cast<SizeType>(rand()) % Size());
  }

  std::string VocabLookup(SizeType idx)
  {
    return vocab_[idx];
  }

  SizeType VocabLookup(std::string &idx)
  {
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

  // naive vector representations - just one-hot encoding on a first come first serve basis
  void BuildTrainingData(std::string &training_data)
  {
    if (cbow_)
    {
      throw std::runtime_error("cbow_ not yet implemented");
    }
    else
    {
      // put all words into an array
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
        }
      }

      // just in case the final word has a full stop or newline - we remove the empty vector
      if (words_.back().size() == 0)
      {
        words_.pop_back();
      }

      assert(word_count_ > (skip_window_ * 2));

      // insert words uniquely into the vocabulary
      SizeType word_counter = 1;  // 0 reserved for unknown word
      vocab_.insert(std::make_pair("UNK", 0));
      vocab_frequency_.insert(std::make_pair("UNK", 0));

      for (std::vector<std::string> &cur_sentence : words_)
      {
        for (std::string cur_word : cur_sentence)
        {
          bool ret = vocab_.insert(std::make_pair(cur_word, word_counter)).second;

          if (ret)
          {
            vocab_frequency_[cur_word] = 1;
            ++word_counter;
          }
          else
          {
            vocab_frequency_[cur_word] += 1;
          }
        }
      }

      SizeType              pos_count = 0;
      SizeType              cur_vocab_idx;
      SizeType              cur_context_idx;
      std::vector<SizeType> input_one_hot(vocab_.size());
      std::vector<SizeType> context_one_hot(vocab_.size());
      SizeType              tmp;

      ////////////////////////////////////////
      /// generate positive training pairs ///
      ////////////////////////////////////////

      // iterate through all sentences
      for (SizeType sntce_idx = 0; sntce_idx < sentence_count_; sntce_idx++)
      {
        // ignore useless short sentences
        if (words_[sntce_idx].size() > ((skip_window_ * 2) + 1))
        {
          for (SizeType i = skip_window_; i < (words_[sntce_idx].size() - skip_window_); i++)
          {
            // current input word idx
            cur_vocab_idx = SizeType(vocab_[words_[sntce_idx][i]]);
            assert(cur_vocab_idx > 0);  // unknown words not yet handled
            assert(cur_vocab_idx < vocab_.size());

            for (SizeType j = 0; j < (2 * skip_window_) + 1; j++)
            {
              if (j != skip_window_)  // i.e. when input != context
              {
                std::string context_word = words_[sntce_idx][i + j - skip_window_];
                if (!(this->DiscardExample(context_word)))
                {
                  // context word idx
                  cur_context_idx = vocab_[context_word];
                  assert(cur_context_idx > 0);  // unknown words not yet handled
                  assert(cur_context_idx < vocab_.size());

                  // build input one hot
                  for (SizeType k = 0; k < vocab_.size(); k++)
                  {
                    tmp = SizeType(k == cur_vocab_idx);
                    assert((tmp == 0) || (tmp == 1));
                    input_one_hot[k] = tmp;
                  }

                  // build context one hot
                  for (SizeType k = 0; k < vocab_.size(); k++)
                  {
                    tmp = SizeType(k == cur_context_idx);
                    assert((tmp == 0) || (tmp == 1));
                    context_one_hot[k] = tmp;
                  }

                  // assign data
                  data_input_.emplace_back(input_one_hot);
                  data_context_.emplace_back(context_one_hot);

                  // assign label
                  labels_.emplace_back(SizeType(1));

                  ++size_;

                  ++pos_count;
                }
              }
            }
          }
        }
      }

      ////////////////////////////////////////
      /// generate negative training pairs ///
      ////////////////////////////////////////

      SizeType neg_count                          = 0;
      SizeType n_negative_training_pairs_per_word = (skip_window_ * 2) * k_negative_samples_;
      SizeType negative_context_idx;

      // iterate through all sentences
      for (SizeType sntce_idx = 0; sntce_idx < sentence_count_; sntce_idx++)
      {
        std::cout << "sntce_idx: " << sntce_idx << std::endl;

        // ignore useless short sentences
        if (words_[sntce_idx].size() > ((skip_window_ * 2) + 1))
        {
          for (SizeType i = skip_window_; i < (words_[sntce_idx].size() - skip_window_); i++)
          {
            // current input word idx
            cur_vocab_idx = vocab_[words_[sntce_idx][i]];
            assert(cur_vocab_idx > 0);
            assert(cur_vocab_idx < vocab_.size());

            for (SizeType j = 0; j < n_negative_training_pairs_per_word; j++)
            {
              // ran select value
              bool ongoing = true;
              while (ongoing)
              {
                negative_context_idx = SizeType(SizeType(rand()) % words_[sntce_idx].size());
                if ((negative_context_idx < (i - skip_window_)) ||
                    (negative_context_idx > (i + skip_window_)))
                {
                  ongoing = false;
                }
              }

              std::string context_word = words_[sntce_idx][negative_context_idx];

              if (!(this->DiscardExample(context_word)))
              {
                negative_context_idx = vocab_[context_word];
                assert(negative_context_idx > 0);
                assert(negative_context_idx < vocab_.size());

                // build input one hot
                for (unsigned int k = 0; k < vocab_.size(); k++)
                {
                  tmp = SizeType(k == cur_vocab_idx);
                  assert((tmp == 0) || (tmp == 1));
                  input_one_hot[k] = tmp;
                }

                // build context one hot
                for (unsigned int k = 0; k < vocab_.size(); k++)
                {
                  tmp = SizeType(k == negative_context_idx);
                  assert((tmp == 0) || (tmp == 1));
                  context_one_hot[k] = tmp;
                }

                if ((pos_count + neg_count) == 1367)
                {
                  for (std::size_t zi = 0; zi < input_one_hot.size(); ++zi)
                  {
                    std::cout << "input_one_hot[zi]: " << input_one_hot[zi] << std::endl;
                  }
                }
                if ((pos_count + neg_count) == 1367)
                {
                  for (std::size_t zi = 0; zi < context_one_hot.size(); ++zi)
                  {
                    std::cout << "context_one_hot[zi]: " << context_one_hot[zi] << std::endl;
                  }
                }

                data_input_.push_back(input_one_hot);
                data_context_.push_back(context_one_hot);

                // assign label
                labels_.push_back(SizeType(0));

                ++size_;
                ++neg_count;
              }
            }
          }
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
    double word_probability = double(vocab_frequency_[word]) / words_.size();
    double prob_thresh      = 1.0 - std::sqrt(discard_threshold_ / word_probability);
    double f                = (double)rand() / RAND_MAX;

    if (f < prob_thresh)
    {
      return true;
    }
    return false;
  }

  SizeType size_ = 0;

  std::unordered_map<std::string, SizeType> vocab_;
  std::unordered_map<std::string, SizeType> vocab_frequency_;
  std::vector<std::vector<std::string>>     words_;

  SizeType cursor_;
  SizeType skip_window_        = 0;
  bool     cbow_               = false;
  SizeType k_negative_samples_ = 1;
  double   discard_threshold_  = 0.00001;

  std::vector<std::vector<SizeType>> data_input_;
  std::vector<std::vector<SizeType>> data_context_;
  std::vector<SizeType>              labels_;

  SizeType sentence_count_ = 0;
  SizeType word_count_     = 0;
};
}  // namespace ml
}  // namespace fetch
