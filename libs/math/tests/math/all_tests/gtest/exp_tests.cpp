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

#include <chrono>
#include <cmath>
#include <iostream>

#include "core/random/lcg.hpp"
#include "math/approx_exp.hpp"
#include <gtest/gtest.h>

template <uint8_t N, uint64_t C>
void test1(double max)
{
  fetch::math::ApproxExpImplementation<N, C> fexp;
  double                                     me = 0;
  for (double x = -300; x < 300; x += 0.1)
  {
    double y0 = fexp(x);
    double y1 = exp(x);
    double r  = fabs(y0 - y1) / y1 * 100;
    me        = std::max(r, me);
  }
  ASSERT_LE(me, max) << "expected: " << max;
}

template <uint8_t N, uint64_t C, std::size_t MAX = 100000000>
double test_timing(double x_value)
{
  fetch::math::ApproxExpImplementation<N, C> fexp;
  volatile double                            x = 1;

  auto t1a = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < MAX; ++i)
  {
    x = x_value;
  }
  auto t2a = std::chrono::high_resolution_clock::now();

  double us1 = double(std::chrono::duration_cast<std::chrono::milliseconds>(t2a - t1a).count());

  auto t1b = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < MAX; ++i)
  {
    x = x_value;
    x = fexp(x);
  }
  auto t2b = std::chrono::high_resolution_clock::now();

  double us2 = double(std::chrono::duration_cast<std::chrono::milliseconds>(t2b - t1b).count());

  auto t1c = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < MAX; ++i)
  {
    x = x_value;
    x = exp(x);
  }
  auto t2c = std::chrono::high_resolution_clock::now();

  double us3 = double(std::chrono::duration_cast<std::chrono::milliseconds>(t2c - t1c).count());

  return (us3 - us1) / (us2 - us1);
  //  auto t3 = std::chrono::high_resolution_clock::now();
}
// TODO(private issue 332): Move to a benchmark

TEST(exp_gtest, testing_exp)
{

  test1<0, 0>(7);
  test1<0, 60801>(5);
  test1<8, 60801>(0.08);
  test1<12, 60801>(0.005);
  test1<16, 60801>(0.0003);
  test1<20, 60801>(0.00004);

  test1<8, 0>(0.08);
  test1<12, 0>(0.005);
  test1<16, 0>(0.0003);
  test1<20, 0>(0.00004);
}