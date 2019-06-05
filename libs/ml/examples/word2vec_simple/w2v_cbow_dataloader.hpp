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
#include "math/tensor.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "unigram_table.hpp"

#include <exception>
#include <map>
#include <random>
#include <string>
#include <utility>

namespace fetch {
namespace ml {

template <typename T>
class CBOWLoader : public DataLoader<fetch::math::Tensor<T>, fetch::math::Tensor<T>>
{
  using SizeType = fetch::math::SizeType;

public:
  using VocabType  = std::map<std::string, std::pair<SizeType, SizeType>>;
  using ReturnType = std::pair<fetch::math::Tensor<T>, fetch::math::Tensor<T>>;

  CBOWLoader(SizeType window_size, SizeType negative_samples)
    : current_sentence_(0)
    , current_word_(0)
    , window_size_(window_size)
    , negative_samples_(negative_samples)
  {}

  SizeType Size() const override
  {
    SizeType size(0);
    for (auto const &s : data_)
    {
      if ((SizeType)s.size() > (2 * window_size_))
      {
        size += (SizeType)s.size() - (2 * window_size_);
      }
    }
    return size;
  }

  bool IsDone() const override
  {
    if (current_sentence_ >= data_.size())
    {
      return true;
    }
    else if (current_sentence_ >= data_.size() - 1)  // In the last sentence
    {
      if (current_word_ > data_.at(current_sentence_).size() - (2 * window_size_ + 1))
      {
        return true;
      }
    }
    return false;
  }

  void Reset() override
  {
    //    std::random_shuffle(data_.begin(), data_.end());
    current_sentence_ = 0;
    current_word_     = 0;
    rng_.Seed(1337);
    unigram_table_.Reset();
  }

  /*
   * Remove words that appears less than MIN times
   * This is a destructive operation
   */
  void RemoveInfrequent(SizeType min)
  {
    // Removing words while keeping indexes consecutive takes too long
    // So creating a new object, not the most efficient, but good enought for now
    CBOWLoader new_loader(window_size_, negative_samples_);
    std::map<SizeType, std::pair<std::string, SizeType>> reverse_vocab;
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
    std::vector<SizeType> frequencies(VocabSize());
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
    SizeType dynamic_size = rng_() % window_size_ + 1;
    t.second.Set(0, 0, T(data_[current_sentence_][current_word_ + dynamic_size]));

    for (SizeType i(0); i < dynamic_size; ++i)
    {
      t.first.Set(i, 0, T(data_[current_sentence_][current_word_ + i]));
      t.first.Set(i + dynamic_size, 0,
                  T(data_[current_sentence_][current_word_ + dynamic_size + i + 1]));
    }

    for (SizeType i(dynamic_size * 2); i < t.first.size(); ++i)
    {
      t.first(i, 0) = -1;
    }

    for (SizeType i(1); i < negative_samples_; ++i)
    {
      t.second(i, 0) = T(unigram_table_.SampleNegative(static_cast<SizeType>(t.second(0, 0))));
    }
    current_word_++;
    if (current_word_ >= data_.at(current_sentence_).size() - (2 * window_size_))
    {
      current_word_ = 0;
      current_sentence_++;
    }
    return t;
  }

  ReturnType GetNext() override
  {
    fetch::math::Tensor<T> t({window_size_ * 2, 1});
    fetch::math::Tensor<T> label({negative_samples_, 1});
    ReturnType             p(t, label);
    return GetNext(p);
  }

  SizeType VocabSize() const
  {
    return vocab_.size();
  }

  bool AddData(std::string const &s)
  {
    std::vector<SizeType> indexes = StringsToIndices(PreprocessString(s));
    if (indexes.size() >= 2 * window_size_ + 1)
    {
      data_.push_back(std::move(indexes));
      return true;
    }
    return false;
  }

  VocabType const &GetVocab() const
  {
    return vocab_;
  }

  std::string WordFromIndex(SizeType index) const
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

  SizeType IndexFromWord(std::string word)
  {
    auto tmp = vocab_[word];
    return tmp.first;
  }

private:
  std::vector<SizeType> StringsToIndices(std::vector<std::string> const &strings)
  {
    std::vector<SizeType> indexes;
    if (strings.size() >= 2 * window_size_ + 1)  // Don't bother processing too short inputs
    {
      indexes.reserve(strings.size());
      for (std::string const &s : strings)
      {
        auto value = vocab_.insert(std::make_pair(s, std::make_pair((SizeType)(vocab_.size()), 0)));
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
  SizeType                                   current_sentence_;
  SizeType                                   current_word_;
  SizeType                                   window_size_;
  SizeType                                   negative_samples_;
  VocabType                                  vocab_;
  std::vector<std::vector<SizeType>>         data_;
  fetch::random::LinearCongruentialGenerator rng_;
  UnigramTable                               unigram_table_;
};
}  // namespace ml
}  // namespace fetch
