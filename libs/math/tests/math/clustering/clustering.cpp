//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include "core/random/lcg.hpp"
#include <math/free_functions/clustering_algorithms/k_means.hpp>
#include <math/linalg/matrix.hpp>

using namespace fetch::math::clustering;
using namespace fetch::math::linalg;

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _M = Matrix<D, _S<D>>;

TEST(distance_tests, clustering_test)
{
  _M<double> A = _M<double>{8, 2};
  A.Set(0, 0, -2);
  A.Set(0, 1, -2);
  A.Set(1, 0, -1);
  A.Set(1, 1, -1);
  A.Set(2, 0, 1);
  A.Set(2, 1, 1);
  A.Set(3, 0, 2);
  A.Set(3, 1, 2);

  // generate random seed
  std::size_t r_seed = 1153486;

  auto output = KMeans(A, 2, r_seed);
  ASSERT_TRUE(output[0] == output[1]);
  ASSERT_TRUE(output[2] == output[3]);
}
