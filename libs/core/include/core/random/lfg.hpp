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

template <std::size_t P = 418, std::size_t Q = 1279>
class LaggedFibonacciGenerator
{
public:
  using random_type = uint64_t;
  LaggedFibonacciGenerator(random_type seed = 42)
  {
    Seed(seed);
  }

  random_type Seed() const
  {
    return lcg_.Seed();
  }
  random_type Seed(random_type const &s)
  {
    random_type ret = lcg_.Seed(s);

    for (std::size_t i = 0; i < Q; ++i)
    {
      buffer_[i] = lcg_();
    }

    FillBuffer();
    FillBuffer();
    FillBuffer();

    return ret;
  }

  void Reset()
  {
    Seed(Seed());
  }

  random_type operator()()
  {
    if (index_ == (Q - 1))
    {
      FillBuffer();
    }
    return buffer_[++index_];
  }

  double AsDouble()
  {
    return double(this->operator()()) * inv_double_max_;
  }

  random_type min()
  {
    return static_cast<random_type>(0);
  }

  random_type max()
  {
    return static_cast<random_type>(std::numeric_limits<random_type>::max());
  }

private:
  void FillBuffer()
  {
    std::size_t j = Q - P;
    std::size_t i = 0;
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

  std::size_t                 index_ = 0;
  LinearCongruentialGenerator lcg_;

  random_type             buffer_[Q];
  static constexpr double inv_double_max_ =
      1. / static_cast<double>(std::numeric_limits<random_type>::max());
};
}  // namespace random
}  // namespace fetch
