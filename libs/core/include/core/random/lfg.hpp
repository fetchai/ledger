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

#include "lcg.hpp"

#include <cstdint>
#include <limits>

namespace fetch {
namespace random {

template <uint64_t P = 418, uint64_t Q = 1279>
class LaggedFibonacciGenerator
{
public:
  using RandomType = uint64_t;

  // Note, breaking naming convention for STL compatibility
  using result_type = RandomType;

  LaggedFibonacciGenerator(RandomType seed = 42) noexcept
  {
    Seed(seed);
  }

  RandomType Seed() const noexcept
  {
    return lcg_.Seed();
  }

  RandomType Seed(RandomType const &s) noexcept
  {
    RandomType ret = lcg_.Seed(s);

    for (uint64_t i = 0; i < Q; ++i)
    {
      buffer_[i] = lcg_();
    }

    FillBuffer();
    FillBuffer();
    FillBuffer();

    return ret;
  }

  void Reset() noexcept
  {
    Seed(Seed());
  }

  RandomType operator()() noexcept
  {
    if (index_ == (Q - 1))
    {
      FillBuffer();
    }
    return buffer_[++index_];
  }

  double AsDouble() noexcept
  {
    return double(this->operator()()) * inv_double_max_;
  }

  static constexpr RandomType min() noexcept
  {
    return static_cast<RandomType>(0);
  }

  static constexpr RandomType max() noexcept
  {
    return static_cast<RandomType>(std::numeric_limits<RandomType>::max());
  }

private:
  void FillBuffer() noexcept
  {
    uint64_t j = Q - P;
    uint64_t i = 0;
    for (; i < P; ++i, ++j)
    {
      buffer_[i] += buffer_[j];
    }

    j = 0;
    for (; i < Q; ++i, ++j)
    {
      buffer_[i] += buffer_[j];
    }

    index_ = 0;
  }

  uint64_t                    index_ = 0;
  LinearCongruentialGenerator lcg_;

  RandomType              buffer_[Q];
  static constexpr double inv_double_max_ =
      1. / static_cast<double>(std::numeric_limits<RandomType>::max());
};
}  // namespace random
}  // namespace fetch
