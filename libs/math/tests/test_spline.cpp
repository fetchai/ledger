//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
#include <math/spline/linear.hpp>
#include <random/lcg.hpp>

// This is to avoid ambiguity in instantation
double dsin(double x) { return sin(x); }
double dcos(double x) { return cos(x); }
double dtan(double x) { return tan(x); }
double dexp(double x) { return exp(x); }

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
  if (me > max)
  {
    std::cout << "expected: " << max << std::endl;
    exit(-1);
  }
}

template <std::size_t N, typename F, std::size_t MAX = 100000000>
double test_timing(F &f, double x_value, double from, double to)
{
  fetch::math::spline::Spline<> spline;
  spline.SetFunction(f, from, to, N);

  volatile double x;

  auto t1a = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < MAX; ++i)
  {
    x = x_value;
  }
  auto t2a = std::chrono::high_resolution_clock::now();

  double us1 = double(std::chrono::duration_cast<std::chrono::milliseconds>(t2a - t1a).count());

  auto            t1b = std::chrono::high_resolution_clock::now();
  volatile double y;
  for (std::size_t i = 0; i < MAX; ++i)
  {
    x = x_value;
    y = spline(x);
  }
  auto t2b   = std::chrono::high_resolution_clock::now();
  x          = y;
  double us2 = double(std::chrono::duration_cast<std::chrono::milliseconds>(t2b - t1b).count());

  auto t1c = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < MAX; ++i)
  {
    x = x_value;
    y = f(x);
  }
  auto t2c = std::chrono::high_resolution_clock::now();

  double us3 = double(std::chrono::duration_cast<std::chrono::milliseconds>(t2c - t1c).count());

  return (us3 - us1) / (us2 - us1);
  //  auto t3 = std::chrono::high_resolution_clock::now();
}

void benchmark()
{
  static fetch::random::LinearCongruentialGenerator gen;
  std::cout << "Benchmarking Sin ... " << std::endl;
  std::cout << test_timing<8>(dsin, gen.AsDouble() * 100, 0, 100) << std::endl;
  std::cout << test_timing<16>(dsin, gen.AsDouble() * 100, 0, 100) << std::endl;
  std::cout << test_timing<20>(dsin, gen.AsDouble() * 100, 0, 100) << std::endl;

  std::cout << "Benchmarking Cos ... " << std::endl;
  std::cout << test_timing<8>(dcos, gen.AsDouble() * 100, 0, 100) << std::endl;
  std::cout << test_timing<16>(dcos, gen.AsDouble() * 100, 0, 100) << std::endl;
  std::cout << test_timing<20>(dcos, gen.AsDouble() * 100, 0, 100) << std::endl;

  std::cout << "Benchmarking Tan ... " << std::endl;
  std::cout << test_timing<8>(dtan, gen.AsDouble() * 100, 0, 100) << std::endl;
  std::cout << test_timing<16>(dtan, gen.AsDouble() * 100, 0, 100) << std::endl;
  std::cout << test_timing<20>(dtan, gen.AsDouble() * 100, 0, 100) << std::endl;

  std::cout << "Benchmarking Exp ... " << std::endl;
  std::cout << test_timing<8>(dexp, gen.AsDouble() * 100, 0, 100) << std::endl;
  std::cout << test_timing<16>(dexp, gen.AsDouble() * 100, 0, 100) << std::endl;
  std::cout << test_timing<20>(dexp, gen.AsDouble() * 100, 0, 100) << std::endl;
}

int main(int argc, char **argv)
{
  if ((argc == 2) && (std::string(argv[1]) == "benchmark")) benchmark();

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

  return 0;
}
