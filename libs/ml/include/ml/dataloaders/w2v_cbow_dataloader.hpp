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

#include "math/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"

#include <exception>
#include <fstream>
#include <map>
#include <string>
#include <utility>

namespace fetch {
namespace ml {

template <typename T>
class CBOWLoader : public DataLoader<fetch::math::Tensor<T>, uint64_t>
{
public:
  CBOWLoader(uint64_t window_size)
    : currentSentence_(0)
    , currentWord_(0)
    , window_size_(window_size)
  {
  }

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
    if (data_.empty())
    {
      return true;
    }
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
    currentSentence_ = 0;
    currentWord_     = 0;
  }

  virtual std::pair<fetch::math::Tensor<T>, uint64_t> GetNext()
  {
    fetch::math::Tensor<T> t(window_size_ * 2);
    uint64_t               label = data_[currentSentence_][currentWord_ + window_size_];
    for (uint64_t i(0); i < window_size_; ++i)
    {
      t.At(i)                = T(data_[currentSentence_][currentWord_ + i]);
      t.At(i + window_size_) = T(data_[currentSentence_][currentWord_ + window_size_ + i + 1]);
    }
    currentWord_++;
    if (currentWord_ >= data_.at(currentSentence_).size() - (2 * window_size_))
    {
      currentWord_ = 0;
      currentSentence_++;
    }
    return std::make_pair(t, label);
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

  std::map<std::string, uint64_t> const &GetVocab() const
  {
    return vocab_;
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
        auto value =
            vocab_.insert(std::pair<std::string, uint64_t>(s, (uint64_t)(vocab_.size())));
        indexes.push_back((*value.first).second);
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
  uint64_t                           currentSentence_;
  uint64_t                           currentWord_;
  uint64_t                           window_size_;
  std::map<std::string, uint64_t>    vocab_;
  std::vector<std::vector<uint64_t>> data_;
};
}  // namespace ml
}  // namespace fetch
