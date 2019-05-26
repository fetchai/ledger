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
#include "dataloader.hpp"
#include "math/tensor.hpp"
#include "unigram_table.hpp"

#include <exception>
#include <fstream>
#include <map>
#include <random>
#include <string>
#include <utility>

namespace fetch {
namespace ml {

template <typename T>
class CBOWLoader : public DataLoader<fetch::math::Tensor<T>, fetch::math::Tensor<T>>
{
public:
  using ReturnType = std::pair<fetch::math::Tensor<T>, fetch::math::Tensor<T>>;

public:
  CBOWLoader(uint64_t window_size, uint64_t negative_samples)
    : currentSentence_(0)
    , currentWord_(0)
    , window_size_(window_size)
    , negative_samples_(negative_samples)
  {}

  virtual uint64_t Size() const
  {
    uint64_t size(0);
    for (auto const &s : data_)
    {
      if ((uint64_t)s.size() > (2 * window_size_))
      {
        size += (uint64_t)s.size() - (2 * window_size_);
      }
    }
    return size;
  }

  virtual bool IsDone() const
  {
    if (currentSentence_ >= data_.size())
    {
      return true;
    }
    else if (currentSentence_ >= data_.size() - 1)  // In the last sentence
    {
      if (currentWord_ > data_.at(currentSentence_).size() - (2 * window_size_ + 1))
      {
        return true;
      }
    }
    return false;
  }

  virtual void Reset()
  {
    //    std::random_shuffle(data_.begin(), data_.end());
    currentSentence_ = 0;
    currentWord_     = 0;
  }

  /*
   * Remove words that appears less than MIN times
   * This is a destructive operation
   */
  void RemoveInfrequent(unsigned int min)
  {
    // Removing words while keeping indexes consecutive takes too long
    // So creating a new object, not the most efficient, but good enought for now
    CBOWLoader new_loader(window_size_, negative_samples_);
    std::map<uint64_t, std::pair<std::string, uint64_t>> reverse_vocab;
    for (auto const &kvp : vocab_)
    {
      reverse_vocab[kvp.second.first] = std::make_pair(kvp.first, kvp.second.second);
    }
    for (auto const &sentence : data_)
    {
      std::string s;
      for (auto const &word : sentence)
      {
        if (reverse_vocab[word].second >= min)
        {
          s += reverse_vocab[word].first + " ";
        }
      }
      new_loader.AddData(s);
    }
    data_  = std::move(new_loader.data_);
    vocab_ = std::move(new_loader.vocab_);
  }

  void InitUnigramTable()
  {
    std::vector<uint64_t> frequencies(VocabSize());
    for (auto const &kvp : GetVocab())
    {
      frequencies[kvp.second.first] = kvp.second.second;
    }
    unigram_table_.Reset(1e8, frequencies);
  }

  ReturnType &GetNext(ReturnType &t)
  {
    // This seems to be one of the most important tricks to get word2vec to train
    // The number of context words changes at each iteration with values in range [1 * 2,
    // window_size_ * 2]
    uint64_t dynamic_size = rng_() % window_size_ + 1;
    t.second.Set(0, 0, T(data_[currentSentence_][currentWord_ + dynamic_size]));

    for (uint64_t i(0); i < dynamic_size; ++i)
    {
      t.first.Set(i, 0, T(data_[currentSentence_][currentWord_ + i]));
      t.first.Set(i + dynamic_size, 0,
                  T(data_[currentSentence_][currentWord_ + dynamic_size + i + 1]));
    }

    for (uint64_t i(dynamic_size * 2); i < t.first.size(); ++i)
    {
      t.first(i, 0) = -1;
    }

    for (uint64_t i(1); i < negative_samples_; ++i)
    {
      t.second(i, 0) = T(unigram_table_.SampleNegative(static_cast<uint64_t>(t.second(0, 0))));
    }
    currentWord_++;
    if (currentWord_ >= data_.at(currentSentence_).size() - (2 * window_size_))
    {
      currentWord_ = 0;
      currentSentence_++;
    }
    return t;
  }

  ReturnType GetNext()
  {
    fetch::math::Tensor<T> t({window_size_ * 2, 1});
    fetch::math::Tensor<T> label({negative_samples_, 1});
    ReturnType             p(t, label);
    return GetNext(p);
  }

  std::size_t VocabSize() const
  {
    return vocab_.size();
  }

  bool AddData(std::string const &s)
  {
    std::vector<uint64_t> indexes = StringsToIndexes(PreprocessString(s));
    if (indexes.size() >= 2 * window_size_ + 1)
    {
      data_.push_back(std::move(indexes));
      return true;
    }
    return false;
  }

  std::map<std::string, std::pair<uint64_t, uint64_t>> const &GetVocab() const
  {
    return vocab_;
  }

  std::string WordFromIndex(uint64_t index)
  {
    for (auto const &kvp : vocab_)
    {
      if (kvp.second.first == index)
      {
        return kvp.first;
      }
    }
    return "";
  }

private:
  std::vector<uint64_t> StringsToIndexes(std::vector<std::string> const &strings)
  {
    std::vector<uint64_t> indexes;
    if (strings.size() >= 2 * window_size_ + 1)  // Don't bother processing too short inputs
    {
      indexes.reserve(strings.size());
      for (std::string const &s : strings)
      {
        auto value = vocab_.insert(std::make_pair(s, std::make_pair((uint64_t)(vocab_.size()), 0)));
        indexes.push_back((*value.first).second.first);
        value.first->second.second++;
      }
    }
    return indexes;
  }

  std::vector<std::string> PreprocessString(std::string const &s)
  {
    std::string result;
    result.reserve(s.size());
    for (auto const &c : s)
    {
      result.push_back(std::isalpha(c) ? (char)std::tolower(c) : ' ');
    }

    std::string              word;
    std::vector<std::string> words;
    for (std::stringstream ss(result); ss >> word;)
    {
      words.push_back(word);
    }
    return words;
  }

private:
  uint64_t                                             currentSentence_;
  uint64_t                                             currentWord_;
  uint64_t                                             window_size_;
  uint64_t                                             negative_samples_;
  std::map<std::string, std::pair<uint64_t, uint64_t>> vocab_;
  std::vector<std::vector<uint64_t>>                   data_;
  fetch::random::LinearCongruentialGenerator           rng_;
  UnigramTable                                         unigram_table_;
};
}  // namespace ml
}  // namespace fetch
