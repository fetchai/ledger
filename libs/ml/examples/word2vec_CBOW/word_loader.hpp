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
#include "core/byte_array/byte_array.hpp"
#include "crypto/fnv.hpp"

#include <vector>
#include <unordered_map>
#include <set>
namespace fetch
{
namespace ml
{

template< typename T >
class WordLoader : public DataLoader<fetch::math::Tensor<T>, fetch::math::Tensor<T>>
{
public:
  using ConstByteArray  = fetch::byte_array::ConstByteArray;
  using ByteArray       = fetch::byte_array::ByteArray;  
  using Tensor          = fetch::math::Tensor<T>;

  void Load(std::string filename)
  {
    std::ifstream t(filename);

    if(!t)
    {
      throw std::runtime_error("could not open " + filename );
    }

    std::string temp((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
    raw_corpus_ = temp;
    corpus_.reserve(1000000);
    frequencies_.reserve(100000);
    Tokenise();
    Reset();
  }

  void LoadString(std::string data)
  {
    raw_corpus_ = data;
    corpus_.reserve(1000);
    frequencies_.reserve(100);

    Tokenise();
    Reset();
  }

  void Reset() override
  {
    current_position_ = 0;
  }


  bool IsDone() const override
  {
    return current_position_ == corpus_.size();
  }

  uint64_t Size() const override
  {
    return word_to_index_.size();
  }
  
  std::pair<Tensor, Tensor> GetNext() override
  {
    std::pair<Tensor, Tensor> ret;
    // This seems to be one of the most important tricks to get word2vec to train
    // The number of context words changes at each iteration with values in range [1 * 2,
    // window_size_ * 2]
    uint64_t dynamic_size = rng_() % window_size_ + 1;

    // Center word goes first
    auto center_word = corpus_[current_position_ + dynamic_size];
    ret.second.Set(0, 0, static_cast<T>(center_word));
    
    // Context in the following words
    uint64_t s = current_position_, t = current_position_ + dynamic_size + 1;
    for (uint64_t i = 0; i < dynamic_size; ++i)
    {
      ret.first.Set(i, 0, static_cast<T>(corpus_[s]));
      ret.first.Set(i + dynamic_size, 0, static_cast<T>(corpus_[t]));
      ++s, ++t;
    }

    for (uint64_t i = dynamic_size * 2; i < ret.first.size(); ++i)
    {
      ret.first(i, 0) = -1;
    }

    // Creating negatice sample
    for (uint64_t i=1; i < negative_samples_; ++i)
    {
      ret.second(i, 0) = static_cast<T>(SampleNegative(center_word));
    }

    // TODO: Divide into sentences

    return ret;
  }

  void SetWindowSize(int64_t w)
  {
    window_size_ = std::move(w);
  }

  void RemoveInfrequent(uint64_t min)
  {  
    // Removing infrequent words
    uint64_t N = corpus_.size();
    uint64_t j = 0, i = 0;
    std::vector< uint64_t > removed_words;
    for( ; i < N; ++i)
    {
      auto word = corpus_[i];
      if(frequencies_[word] > min)
      {
        ++j;
      }
      else
      {
        removed_words.push_back(word);
      }
      corpus_[j] = std::move(word);
    }

    corpus_.resize(j);

    // Reindexing
    /*
    frequencies_.resize(corpus_.size());
    for(auto &f: frequencies_)
    {
      f = 0;
    }

    for(auto &w: corpus_)
    {
      ++frequencies_[w];
    }
    */
  }

  void InitUnigramTable(uint64_t samples)
  {
    word_distribution_.resize(samples);

    double total = 0.0;
    for (auto const &e : frequencies_)
    {
      total += std::pow(e, 0.75);
    }

    uint64_t i = 0;
    double      n = pow(frequencies_[i], 0.75) / total;
    double rec = 1.0 / static_cast<double>(samples);
    for (uint64_t j=0; j < samples; ++j)
    {
      word_distribution_[j] = i;
      if (j * rec > n)
      {
        i++;
        n += pow(frequencies_[i], 0.75) / total;
      }
    }

    assert(i == frequencies_.size() - 1);
  }

  uint64_t Sample()
  {
    return word_distribution_[(rng_()>>19) % word_distribution_.size()];
  }

  uint64_t SampleNegative(uint64_t positiveIndex)
  {
    uint64_t sample = word_distribution_[rng_() % word_distribution_.size()];
    while (sample == positiveIndex)
    {
      sample = word_distribution_[(rng_() >> 19) % word_distribution_.size()];
    }
    return sample;
  }
private:

  void Tokenise()
  {
    uint64_t last_space = 0;
    uint64_t pos = 1;

    while(pos < raw_corpus_.size())
    {      
      if(raw_corpus_[pos] == ' ')
      {
        InsertWord(last_space, pos);
        last_space = pos + 1;
      }

      ++pos;
    }

    if(pos > last_space)
    {
      InsertWord(last_space, pos);
    }
  }

  void InsertWord(uint64_t last_space, uint64_t pos)
  {
    auto word = raw_corpus_.SubArray(last_space, pos - last_space);

    auto it = word_to_index_.find(word);
    if(it == word_to_index_.end())
    {
      corpus_.emplace_back(frequencies_.size());
      word_to_index_.insert({word, frequencies_.size()});
      index_to_word_.insert({frequencies_.size(), word});
      frequencies_.emplace_back(1);
    }
    else
    {
      corpus_.emplace_back(it->second);      
      ++frequencies_[ it->second ];
    }
  }

  ByteArray raw_corpus_;

  std::vector< uint64_t > corpus_;
  std::vector< uint64_t > frequencies_;
  std::vector< uint64_t > word_distribution_;

  std::unordered_map< ConstByteArray, uint64_t > word_to_index_;
  std::unordered_map< uint64_t, ConstByteArray > index_to_word_;  
  fetch::random::LinearCongruentialGenerator rng_;

  uint64_t current_position_{0};
  uint64_t window_size_{8};
  uint64_t negative_samples_{8};  
};

}
}
