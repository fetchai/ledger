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
#include <benchmark/benchmark.h>
#include <cmath>
#include <iostream>

using namespace fetch::memory;

template <typename type, unsigned long N = 100000>
class ParallelDispatcherSSEBench : public ::benchmark::Fixture
{
public:
  using ndarray_type       = SharedArray<type>;
  using VectorRegisterType = typename ndarray_type::VectorRegisterType;

protected:
  void SetUp(const ::benchmark::State & /*st*/) override
  {
    a_ = ndarray_type(N);
    b_ = ndarray_type(N);
    for (std::size_t i = 0; i < N; ++i)
    {
      b_[i] = type(i);
    }
  }

  void TearDown(const ::benchmark::State &) override
  {}

  ndarray_type        a_, b_;
  const unsigned long MAX_ = N;
};
BENCHMARK_TEMPLATE_F(ParallelDispatcherSSEBench, Standard_implementation, float)
(benchmark::State &st)
{
  // Standard implementation
  for (auto _ : st)
  {
    for (std::size_t j = 0; j < MAX_; j += 4)
    {

      // We write it out such that the compiler might use SSE
      a_[j]     = std::exp(1 + std::log(b_[j]));
      a_[j + 1] = std::exp(1 + std::log(b_[j + 1]));
      a_[j + 2] = std::exp(1 + std::log(b_[j + 2]));
      a_[j + 3] = std::exp(1 + std::log(b_[j + 3]));
    }
  }
}
BENCHMARK_MAIN();
