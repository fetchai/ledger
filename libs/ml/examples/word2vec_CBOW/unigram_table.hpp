#pragma once

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
