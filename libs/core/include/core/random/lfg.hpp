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

#include "vectorise/fixed_point/fixed_point.hpp"
#include <cstdint>
#include <limits>
#include <vector>

namespace fetch {
namespace random {

template <uint64_t P = 418, uint64_t Q = 1279>
class LaggedFibonacciGenerator
{
public:
  using RandomType = uint64_t;
  using fp64_t     = fetch::fixed_point::fp64_t;

  // Note, breaking naming convention for STL compatibility
  using result_type = RandomType;

  explicit LaggedFibonacciGenerator(RandomType seed = 42) noexcept
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
      buffer_[i] = (lcg_() >> 19) ^ lcg_();
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

  fp64_t AsFP64() noexcept
  {
    return static_cast<fp64_t>(this->operator()()) * inv_fp64_max_;
  }

  static constexpr RandomType min() noexcept
  {
    return static_cast<RandomType>(0);
  }

  static constexpr RandomType max() noexcept
  {
    return static_cast<RandomType>(std::numeric_limits<RandomType>::max());
  }

  /**
   * required for serialising lfg
   * @return
   */
  std::vector<RandomType> GetBuffer() const
  {
    return std::vector<RandomType>(std::begin(buffer_), std::end(buffer_));
  }

  void SetBuffer(std::vector<RandomType> const &buffer)
  {
    for (uint64_t i = 0; i < Q; ++i)
    {
      buffer_[i] = buffer[i];
    }
  }

  uint64_t GetIndex() const
  {
    return index_;
  }

  void SetIndex(uint64_t index)
  {
    index_ = index;
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

  RandomType              buffer_[Q]{};
  static constexpr double inv_double_max_ =
      1. / static_cast<double>(std::numeric_limits<RandomType>::max());

  static const fp64_t inv_fp64_max_;
};

template <uint64_t P, uint64_t Q>
const fetch::fixed_point::fp64_t LaggedFibonacciGenerator<P, Q>::inv_fp64_max_ =
    static_cast<fp64_t>(1) / static_cast<fp64_t>(std::numeric_limits<RandomType>::max());

}  // namespace random
}  // namespace fetch
