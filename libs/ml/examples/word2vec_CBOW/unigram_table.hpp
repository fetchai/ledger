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

class UnigramTable
{
public:
  UnigramTable(unsigned int size = 0, std::vector<uint64_t> const &frequencies = {})
  {
    Reset(size, frequencies);
  }

  void Reset(unsigned int size, std::vector<uint64_t> const &frequencies)
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
        if (j / (double)size > n)
        {
          i++;
          n += pow(frequencies[i], 0.75) / total;
        }
      }
      assert(i == frequencies.size() - 1);
    }
  }

  uint64_t Sample()
  {
    return data_[rng_() % data_.size()];
  }

  uint64_t SampleNegative(uint64_t positiveIndex)
  {
    uint64_t sample = data_[rng_() % data_.size()];
    while (sample == positiveIndex)
    {
      sample = data_[rng_() % data_.size()];
    }
    return sample;
  }

private:
  std::vector<uint64_t>                      data_;
  fetch::random::LinearCongruentialGenerator rng_;
};
