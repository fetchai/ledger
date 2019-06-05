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

namespace fetch {
namespace ml {

class UnigramTable
{
public:
  UnigramTable(unsigned int size = 0, std::vector<uint64_t> const &frequencies = {});

  void     Reset(unsigned int size, std::vector<uint64_t> const &frequencies);
  uint64_t Sample();
  uint64_t SampleNegative(uint64_t positive_index);
  void     Reset();

private:
  std::vector<uint64_t>                      data_;
  fetch::random::LinearCongruentialGenerator rng_;
};

void UnigramTable::Reset(unsigned int size, std::vector<uint64_t> const &frequencies)
{
  if (size && frequencies.size())
  {
    data_.resize(size);
    double total(0);
    for (auto const &e : frequencies)
    {
      total += std::pow(e, 0.75);
    }
    std::size_t i(0);
    double      n = pow(frequencies[i], 0.75) / total;
    for (std::size_t j(0); j < size; ++j)
    {
      data_[j] = i;
      if (static_cast<double>(j) / static_cast<double>(size) > static_cast<double>(n))
      {
        i++;
        n += pow(frequencies[i], 0.75) / total;
      }
    }
    assert(i == frequencies.size() - 1);
  }
}

/**
 * constructor specifies table size and vector of frequencies to prefill
 * @param size size of the table
 * @param frequencies vector of frequencies used to calculate how many rows per index
 */
UnigramTable::UnigramTable(unsigned int size, std::vector<uint64_t> const &frequencies)
{
  Reset(size, frequencies);
}

/**
 * samples a random data point from the unigram table
 * @return
 */
uint64_t UnigramTable::Sample()
{
  return data_[rng_() % data_.size()];
}

/**
 * samples a negative value from unigram table, method is unsafe (could loop forever)
 * @param positive_index
 * @return
 */
uint64_t UnigramTable::SampleNegative(uint64_t positive_index)
{
  // TODO: make safe - either include a timeout or check if any negative indices exist
  uint64_t sample = data_[rng_() % data_.size()];
  while (sample == positive_index)
  {
    sample = data_[rng_() % data_.size()];
  }
  return sample;
}

/**
 * resets random number generation for sampling
 */
void UnigramTable::Reset()
{
  rng_.Seed(42 * 1337);
}

}  // namespace ml
}  // namespace fetch