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

#include <chrono>
#include <cmath>
#include <iostream>
#include <math/exp.hpp>
#include <random/lcg.hpp>

template <std::size_t N, std::size_t C>
void test1(double max)
{
  fetch::math::Exp<N, C> fexp;
  double                 me = 0;
  for (double x = -300; x < 300; x += 0.1)
  {
    double y0 = fexp(x);
    double y1 = exp(x);
    double r  = fabs(y0 - y1) / y1 * 100;
    me        = std::max(r, me);
  }
  std::cout << "Peak error: " << me << std::endl;
  if (me > max)
  {
    std::cout << "expected: " << max << std::endl;
    exit(-1);
  }
}

int main(int argc, char **argv)
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
  return 0;
}
