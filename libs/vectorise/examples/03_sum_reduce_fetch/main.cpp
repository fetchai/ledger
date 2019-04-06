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

#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

using type        = double;
using array_type  = fetch::memory::SharedArray<type>;
using vector_type = typename array_type::vector_register_type;

type InnerProduct(array_type const &A, array_type const &B)
{
  type ret = 0;

  ret = A.in_parallel().SumReduce(
      [](vector_type const &a, vector_type const &b) {
        vector_type d = a - b;
        return d * d;
      },
      B);

  return ret;
}

int main(int argc, char const **argv)
{
  if (argc != 2)
  {
    std::cout << std::endl;
    std::cout << "Usage: " << argv[0] << " [array size] " << std::endl;
    std::cout << std::endl;
    return 0;
  }

  std::size_t N = std::size_t(atoi(argv[1]));

  array_type A(N), B(N);

  for (std::size_t i = 0; i < N; ++i)
  {
    A[i] = type(0.001 * std::sin(-0.1 * type(i)));
    B[i] = type(0.001 * std::cos(-0.1 * type(i)));
  }

  std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  type                                           ret;
  for (std::size_t i = 0; i < 10000; ++i)
  {
    ret = InnerProduct(A, B);
  }
  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
  double time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();

  std::cout << time_span << " s to get " << ret << std::endl;

  return 0;
}
