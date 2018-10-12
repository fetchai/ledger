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

#include <iomanip>
#include <iostream>

#include "core/random/lcg.hpp"
#include <gtest/gtest.h>
#include <math/distance/manhattan.hpp>
#include <math/linalg/matrix.hpp>

using namespace fetch::math::distance;
using namespace fetch::math::linalg;

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _M = Matrix<D, _S<D>>;

  TEST(manhattan_gtest , basic_info)
  {
    _M<double> A = _M<double>(R"(1 0 0)");
    EXPECT_EQ(0 , Manhattan(A, A));

    _M<double> B = _M<double>(R"(0 1 0)");
    EXPECT_EQ(Manhattan(A, B) , 2);

    B = _M<double>(R"(0 2 0)");
    EXPECT_EQ(Manhattan(A, B) , 3);

    B = _M<double>(R"(1 1 0)");
    EXPECT_EQ(Manhattan(A, B) , 1);
  }