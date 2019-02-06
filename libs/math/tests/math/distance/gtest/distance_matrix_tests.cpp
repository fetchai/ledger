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

#include <iomanip>
#include <iostream>

#include "core/random/lcg.hpp"
#include "math/distance/distance_matrix.hpp"
#include <gtest/gtest.h>
#include <math/distance/hamming.hpp>

using namespace fetch::math::distance;
// using namespace fetch::math::linalg;

// template <typename D>
// using _S = fetch::memory::SharedArray<D>;

// template <typename D>
// using _M = Matrix<D, _S<D>>;

TEST(distance_matrix_gtest, DISABLED_basic_info)
{
  // _M<double> A, B, R;

  // R.Resize(3, 3);
  // A = _M<double>(R"(1 2 3 ; 1 1 1 ; 2 1 2)");
  // B = _M<double>(R"(1 2 9 ; 1 0 0 ; 1 2 3)");

  // EXPECT_TRUE(
  //     bool(DistanceMatrix(R, A, B, Hamming<double>) == _M<double>(R"( 2 1 3 ; 1 1 1  ; 0 0 0
  //     )")));
}
