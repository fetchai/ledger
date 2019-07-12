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

#include "core/random/lcg.hpp"
#include "math/spline/linear.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// This is to avoid ambiguity in instantation
double dsin(double x)
{
  return sin(x);
}
double dcos(double x)
{
  return cos(x);
}
double dtan(double x)
{
  return tan(x);
}
double dexp(double x)
{
  return exp(x);
}
template <std::size_t N, typename F>
void test1(F &f, double from, double to, double max)
{
  fetch::math::spline::Spline<> spline;
  spline.SetFunction(f, from, to, N);
  double me = 0;
  for (double x = from; x < to; x += 0.0001)
  {
    double y0 = spline(x);
    double y1 = f(x);
    double r  = fabs(y0 - y1) / y1 * 100;
    me        = std::max(r, me);
  }
  std::cout << "Peak error: " << me << std::endl;
  ASSERT_LE(me, max) << "expected: " << max;
}
// TODO(private issue 332): Move to a benchmark
TEST(spline_gtest, testing_spline)
{
  std::cout << "Testing Sin ... " << std::endl;
  test1<8>(dsin, 0, 2 * 3.14, 2);
  test1<16>(dsin, 0, 2 * 3.14, 4e-5);
  test1<20>(dsin, 0, 2 * 3.14, 4e-5);
  std::cout << "Testing Cos ... " << std::endl;
  test1<8>(dcos, 0, 2 * 3.14, 2);
  test1<16>(dcos, 0, 2 * 3.14, 4e-5);
  test1<20>(dcos, 0, 2 * 3.14, 4e-4);
  std::cout << "Testing Tan ... " << std::endl;
  test1<20>(dtan, -3.14 / 2., 3.14 / 2., 0.002);
  test1<16>(dtan, -3.14 / 2., 3.14 / 2., 0.05);
  test1<8>(dtan, -3.14 / 2., 3.14 / 2., 400);
  std::cout << "Testing Exp ... " << std::endl;
  test1<8>(dexp, 0, 100, 2);
  test1<16>(dexp, 0, 100, 4e-5);
  test1<20>(dexp, 0, 100, 4e-5);
}
