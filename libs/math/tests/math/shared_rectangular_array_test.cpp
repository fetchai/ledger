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

#include <cstdint>
#include <iostream>
#include <vector>

#include "core/random/lcg.hpp"
#include "math/rectangular_array.hpp"

using namespace fetch::math;

using data_type  = double;
using array_type = RectangularArray<data_type>;

void test1()
{
  std::vector<double>                               dataset;
  static fetch::random::LinearCongruentialGenerator gen;
  std::size_t                                       N = gen() % 5000;
  std::size_t                                       M = gen() % 5000;

  array_type mem(N, M);

  if ((N != mem.height()) || (M != mem.width()))
  {
    std::cout << "size mismatch I " << std::endl;
    exit(-1);
  }
  for (std::size_t i = 0; i < N; ++i)
  {
    for (std::size_t j = 0; j < M; ++j)
    {
      data_type d = gen.AsDouble();
      mem(i, j)   = d;
      dataset.push_back(d);
    }
  }

  std::size_t k = 0;
  for (std::size_t i = 0; i < N; ++i)
  {
    for (std::size_t j = 0; j < M; ++j)
    {
      if (mem(i, j) != dataset[k])
      {
        std::cout << "Data differs!" << std::endl;
        exit(-1);
      }
      ++k;
    }
  }

  for (std::size_t i = 0; i < N * M; ++i)
  {
    if (mem[i] != dataset[i])
    {
      std::cout << "Data differs II!" << std::endl;
      exit(-1);
    }
  }

  array_type mem2(mem);
  if ((mem2.height() != mem.height()) || (mem2.width() != mem.width()))
  {
    std::cout << "size mismatch II " << std::endl;
    exit(-1);
  }
  k = 0;
  for (std::size_t i = 0; i < N; ++i)
  {
    for (std::size_t j = 0; j < M; ++j)
    {
      if (mem2(i, j) != dataset[k])
      {
        std::cout << "Data differs III!" << std::endl;
        exit(-1);
      }
      ++k;
    }
  }

  for (std::size_t i = 0; i < N * M; ++i)
  {
    if (mem2[i] != dataset[i])
    {
      std::cout << "Data differs IV!" << std::endl;
      exit(-1);
    }
  }

  array_type mem3;
  mem3 = mem;
  if ((mem3.height() != mem.height()) || (mem3.width() != mem.width()))
  {
    std::cout << "size mismatch III " << std::endl;
    exit(-1);
  }
  k = 0;
  for (std::size_t i = 0; i < N; ++i)
  {
    for (std::size_t j = 0; j < M; ++j)
    {
      if (mem3(i, j) != dataset[k])
      {
        std::cout << "Data differs V!" << std::endl;
        exit(-1);
      }
      ++k;
    }
  }

  for (std::size_t i = 0; i < N * M; ++i)
  {
    if (mem3[i] != dataset[i])
    {
      std::cout << "Data differs VI!" << std::endl;
      exit(-1);
    }
  }
}

int main()
{
  test1();

  return 0;
}
