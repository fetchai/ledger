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

#include "benchmark/benchmark.h"

#include <cmath>
#include <cstddef>

using namespace fetch::memory;

template <typename type, unsigned long N = 100000>
class ParallelDispatcherKernelBench : public ::benchmark::Fixture
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

  ndarray_type a_, b_;
}; 
BENCHMARK_TEMPLATE_F(ParallelDispatcherKernelBench, kernel_implementation, double)
(benchmark::State &st)
{
  // Standard implementation
  for (auto _ : st)
  {
    // Here we use a kernel to compute the same, using an approximation
    a_.in_parallel().Apply(
        [](VectorRegisterType const &x, VectorRegisterType &y) {
          static VectorRegisterType one(1);

          // We approximate the exponential function by a clever first order
          // Taylor expansion
          y = approx_exp(one + approx_log(x));
        },
        b_);
  }
}
