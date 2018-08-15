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

#include "storage/random_access_stack.hpp"
#include "core/random/lfg.hpp"

#include <iostream>
#include <stack>
using namespace fetch::storage;

template <typename T>
void SimpleTest(std::size_t N = 100)
{
  RandomAccessStack<T>                      stack;
  std::stack<T>                             reference;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  stack.New("random_access_stack_test_1.db");
  for (std::size_t i = 0; i < N; ++i)
  {
    uint64_t val = lfg();
    reference.push(val);
    stack.Push(val);
  }

  for (std::size_t i = 0; i < N; ++i)
  {
    uint64_t a = reference.top();
    uint64_t b = stack.Top();
    std::cout << a << " " << b << std::endl;
    if (a != b)
    {
      std::cerr << "failed as " << a << " != " << b << std::endl;
      exit(-1);
    }
    reference.pop();
    stack.Pop();
    if (reference.size() != stack.size())
    {
      std::cerr << "size mismatch" << std::endl;
      exit(-1);
    }
  }
}

int main()
{
  SimpleTest<uint64_t>();
  return 0;
}
