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
  W2VLoader(std::string &data, SizeType skip_window, bool cbow)
    : cursor_(0)
  {
    assert(skip_window > 0);

    // set up training dataset
    BuildTrainingData(data, skip_window, cbow);
  }

  virtual SizeType Size() const
  {
    return size_;
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
    std::shared_ptr<T> input_buffer   = std::make_shared<T>(std::vector<SizeType>({vocab_.size()}));
    std::shared_ptr<T> context_buffer = std::make_shared<T>(std::vector<SizeType>({vocab_.size()}));

    // input word
    for (SizeType i(0); i < vocab_.size(); ++i)
    {
      input_buffer->At(i) = typename T::Type(data_[idx].first[i]);
    }

    // context word
    for (SizeType i(0); i < vocab_.size(); ++i)
    {
      context_buffer->At(i) = typename T::Type(data_[idx].second[i]);
    }

    SizeType label = (SizeType)(labels_[idx]);
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
    return GetAtIndex((SizeType)rand() % Size());
  }

private:
  // naive vector representations - just one-hot encoding on a first come first serve basis
  void BuildTrainingData(std::string &training_data, SizeType skip_window, bool cbow)
  {
    if (cbow)
    {
      throw std::runtime_error("CBOW not yet implemented");
    }
    else
    {
      // put all words into an array
      std::string              word;
      std::vector<std::string> words;
      for (std::stringstream s(training_data); s >> word;)
      {
        words.push_back(word);
      }

      assert(words.size() > (skip_window * 2));

      // insert words uniquely into the vocabulary
      for (std::uint64_t i = 0; i < words.size(); i++)
      {
        vocab_.insert(std::pair<std::string, SizeType>(words[i], i));
      }

      // generate training pairs
      SizeType n_positive_training_pairs = ((words.size() - (2 * skip_window)) * 2 * skip_window);
      SizeType n_negative_training_pairs = n_positive_training_pairs;
      SizeType n_training_pairs          = n_positive_training_pairs + n_negative_training_pairs;
      size_                              = n_training_pairs;

      // now we know the size of the training data
      data_.resize(n_training_pairs);
      labels_.resize(n_training_pairs);

      // generate positive training pairs
      SizeType pos_count = 0;
      for (SizeType i = skip_window; i < (words.size() - skip_window); i++)
      {
        // current input word idx
        SizeType cur_vocab_idx = vocab_[words[i]];

        for (SizeType j = 0; j < (2 * skip_window) + 1; j++)
        {
          if (j != skip_window)  // i.e. when input == context
          {
            // context word idx
            SizeType cur_context_idx = vocab_[words[i + j - skip_window]];

            // build input one hot
            std::vector<SizeType> input_one_hot{vocab_.size()};
            std::vector<SizeType> context_one_hot{vocab_.size()};
            for (unsigned int k = 0; k < vocab_.size(); k++)
            {
              input_one_hot.emplace_back(SizeType(k == cur_vocab_idx));
            }

            // build context one hot
            for (unsigned int k = 0; k < vocab_.size(); k++)
            {
              context_one_hot.emplace_back(SizeType(k == cur_context_idx));
            }

            data_[pos_count] = std::pair<std::vector<SizeType>, std::vector<SizeType>>(
                input_one_hot, context_one_hot);

            // assign label
            labels_[pos_count] = SizeType(1);

            ++pos_count;
          }
        }
      }

      // generate negative training pairs
      SizeType neg_count                          = 0;
      SizeType n_negative_training_pairs_per_word = skip_window * 2;
      for (SizeType i = skip_window; i < (words.size() - skip_window); i++)
      {
        // current input word idx
        SizeType cur_vocab_idx = vocab_[words[i]];

        for (SizeType j = 0; j < n_negative_training_pairs_per_word; j++)
        {
          // ran select value
          bool     ongoing = true;
          SizeType negative_context_idx;
          while (ongoing)
          {
            negative_context_idx = SizeType(SizeType(rand()) % words.size());
            if ((negative_context_idx < (i - skip_window)) ||
                (negative_context_idx > (i + skip_window)))
            {
              ongoing = false;
            }
          }

          // build input one hot
          std::vector<SizeType> input_one_hot{vocab_.size()};
          std::vector<SizeType> context_one_hot{vocab_.size()};
          for (unsigned int k = 0; k < vocab_.size(); k++)
          {
            input_one_hot.emplace_back(SizeType(k == cur_vocab_idx));
          }

          // build context one hot
          for (unsigned int k = 0; k < vocab_.size(); k++)
          {
            context_one_hot.emplace_back(SizeType(k == negative_context_idx));
          }

          data_[pos_count + neg_count] = std::pair<std::vector<SizeType>, std::vector<SizeType>>(
              input_one_hot, context_one_hot);

          // assign label
          labels_[pos_count + neg_count] = SizeType(0);
          ++neg_count;
        }
      }
    }
  }

  SizeType                                  size_;
  std::unordered_map<std::string, SizeType> vocab_;

  SizeType cursor_;

  std::vector<std::pair<std::vector<SizeType>, std::vector<SizeType>>> data_;
  std::vector<SizeType>                                                labels_;
};
}  // namespace ml
}  // namespace fetch
