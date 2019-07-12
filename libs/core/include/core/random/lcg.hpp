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

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <utility>

namespace fetch {
namespace random {
class LinearCongruentialGenerator
{
public:
  using RandomType = uint64_t;

  // Note, breaking naming convention for STL compatibility  
  using result_type = RandomType;

  LinearCongruentialGenerator(RandomType seed = 42) noexcept
  {
    Seed(std::move(seed));
  }

  RandomType Seed() const noexcept
  {
    return seed_;
  }

  RandomType Seed(RandomType const &s) noexcept
  {
    return x_ = seed_ = s;
  }

  void Reset() noexcept
  {
    Seed(Seed());
  }

  RandomType operator()() noexcept
  {
    return x_ = x_ * a_ + c_;
  }

  double AsDouble() noexcept
  {
    return static_cast<double>(this->operator()()) * inv_double_max_;
  }

  static constexpr RandomType max() noexcept
  {
    return std::numeric_limits<RandomType>::max();
  }

  static constexpr RandomType min() noexcept
  {
    return std::numeric_limits<RandomType>::min();
  }

private:
  RandomType x_    = 1;
  RandomType seed_ = 1;
  RandomType a_    = 6364136223846793005ull;
  RandomType c_    = 1442695040888963407ull;

  static constexpr double inv_double_max_ = 1. / double(std::numeric_limits<RandomType>::max());
};
}  // namespace random
}  // namespace fetch
