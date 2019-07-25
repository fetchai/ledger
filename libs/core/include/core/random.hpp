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

#include "core/random/lfg.hpp"

#include <vector>

namespace fetch {
namespace random {

struct Random
{
  static LaggedFibonacciGenerator<> generator;
};


/**
 * Fisher-Yates shuffle algorithm for lfg
 * @tparam T
 */
template <typename T>
static void Shuffle(LaggedFibonacciGenerator<> & gen, std::vector<T> const &in_vec, std::vector<T> &out_vec)
{
  out_vec = in_vec;

  for (std::size_t i = out_vec.size() - 1; i > 0; --i)
  {
    std::size_t j = (gen() >> 19) % (i + 1);

    // swap i and j
    T temp     = out_vec[i];
    out_vec[i] = out_vec[j];
    out_vec[j] = temp;
  }
}

/**
 * Fisher-Yates shuffle algorithm for lcg
 * @tparam T
 */
template <typename T>
static void Shuffle(LinearCongruentialGenerator & gen, std::vector<T> const &in_vec, std::vector<T> &out_vec)
{
  out_vec = in_vec;

  for (std::size_t i = out_vec.size() - 1; i > 0; --i)
  {
    std::size_t j = (gen() >> 19) % (i + 1);

    // swap i and j
    T temp     = out_vec[i];
    out_vec[i] = out_vec[j];
    out_vec[j] = temp;
  }
}
}  // namespace random
}  // namespace fetch
