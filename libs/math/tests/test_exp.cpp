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

template <std::size_t N, std::size_t C, std::size_t MAX = 100000000>
double test_timing(double x_value)
{
  fetch::math::Exp<N, C> fexp;
  volatile double        x = 1;

  auto t1a = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < MAX; ++i)
  {
    x = x_value;
  }
  auto t2a = std::chrono::high_resolution_clock::now();

  double us1 = double(
      std::chrono::duration_cast<std::chrono::milliseconds>(t2a - t1a).count());

  auto t1b = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < MAX; ++i)
  {
    x = x_value;
    x = fexp(x);
  }
  auto t2b = std::chrono::high_resolution_clock::now();

  double us2 = double(
      std::chrono::duration_cast<std::chrono::milliseconds>(t2b - t1b).count());

  auto t1c = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < MAX; ++i)
  {
    x = x_value;
    x = exp(x);
  }
  auto t2c = std::chrono::high_resolution_clock::now();

  double us3 = double(
      std::chrono::duration_cast<std::chrono::milliseconds>(t2c - t1c).count());

  return (us3 - us1) / (us2 - us1);
  //  auto t3 = std::chrono::high_resolution_clock::now();
}

void benchmark()
{
  static fetch::random::LinearCongruentialGenerator gen;
  std::cout << "Test time 1: ";
  for (std::size_t i = 0; i < 10; ++i)
    std::cout << test_timing<0, 0>(gen.AsDouble() * 100) << " " << std::flush;

  std::cout << "Test time 2: ";
  for (std::size_t i = 0; i < 10; ++i)
    std::cout << test_timing<8, 60801>(gen.AsDouble() * 100) << " "
              << std::flush;

  std::cout << "Test time 3: ";
  for (std::size_t i = 0; i < 10; ++i)
    std::cout << test_timing<12, 60801>(gen.AsDouble() * 100) << " "
              << std::flush;

  std::cout << "Test time 4: ";
  for (std::size_t i = 0; i < 16; ++i)
    std::cout << test_timing<12, 60801>(gen.AsDouble() * 100) << " "
              << std::flush;
}

int main(int argc, char **argv)
{
  if ((argc == 2) && (std::string(argv[1]) == "benchmark")) benchmark();

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
