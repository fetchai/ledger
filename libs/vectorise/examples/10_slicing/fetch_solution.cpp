//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include <cstddef>
#include <cstdlib>
#include <iostream>

using type        = double;
using array_type  = fetch::memory::SharedArray<type>;
using vector_type = typename array_type::VectorRegisterType;

void SlicedOperations(array_type const &A, array_type &C)
{
  auto aslice = A.slice(8, 6);
  C.slice(4, 6).in_parallel().Apply([](auto const &a, auto &c) { c = a; }, aslice);
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

  auto       N = std::size_t(std::atoi(argv[1]));
  array_type A(N), C(N);

  for (std::size_t i = 0; i < N; ++i)
  {
    A[i] = double(i);
    C[i] = 0;
  }

  std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
  //  for(std::size_t i = 0; i < 10000; ++i) {
  SlicedOperations(A, C);
  //  }
  for (std::size_t i = 0; i < N; ++i)
  {
    std::cout << A[i] << " ";
  }
  std::cout << std::endl;
  for (std::size_t i = 0; i < N; ++i)
  {
    std::cout << C[i] << " ";
  }
  std::cout << std::endl;

  std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
  double time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();

  std::cout << time_span << " s" << std::endl;

  return 0;
}
