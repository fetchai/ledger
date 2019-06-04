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

#include "vectorise/memory/shared_array.hpp"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <type_traits>
#include <vector>

using type       = float;
using array_type = std::vector<type>;

void RelativeDifference(array_type const &A, array_type const &B, array_type &C)
{
  for (std::size_t i = 0; i < A.size(); ++i)
  {
    type a = A[i];
    type b = B[i];
    C[i]   = type(0.5) * (a - b) / (a + b);
  }
}

int main(int argc, char const **argv)
{
  std::vector<type> A, B, C;

  if (argc != 2)
  {
    std::cout << std::endl;
    std::cout << "Usage: " << argv[0] << " [array size] " << std::endl;
    std::cout << std::endl;
    return 0;
  }

  auto N = std::size_t(std::atoi(argv[1]));
  A.resize(N);
  B.resize(N);
  C.resize(N);

  for (std::size_t i = 0; i < N; ++i)
  {
    A[i] = type(i);
    B[i] = 2 * type(i);
  }

  std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < 10000; ++i)
  {
    RelativeDifference(A, B, C);
  }
  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
  double time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();

  std::cout << time_span << " s" << std::endl;

  return 0;
}
