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
  using random_type = uint64_t;

  LinearCongruentialGenerator(random_type seed = 42)
  {
    Seed(std::move(seed));
  }

  random_type Seed() const
  {
    return seed_;
  }
  random_type Seed(random_type const &s)
  {
    return x_ = seed_ = s;
  }
  void Reset()
  {
    Seed(Seed());
  }
  random_type operator()()
  {
    return x_ = x_ * a_ + c_;
  }
  double AsDouble()
  {
    return double(this->operator()()) * inv_double_max_;
  }

  static constexpr random_type max()
  {
    return std::numeric_limits<random_type>::max();
  }

  static constexpr random_type min()
  {
    return std::numeric_limits<random_type>::min();
  }

private:
  random_type x_    = 1;
  random_type seed_ = 1;
  random_type a_    = 6364136223846793005ull;
  random_type c_    = 1442695040888963407ull;

  static constexpr double inv_double_max_ = 1. / double(std::numeric_limits<random_type>::max());
};
}  // namespace random
}  // namespace fetch
