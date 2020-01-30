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

#include "vectorise/math/standard_functions.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"
#include "vectorise/vectorise.hpp"

#include "benchmark/benchmark.h"

#include <cmath>
#include <cstddef>

using namespace fetch::memory;

template <typename type, uint64_t N = 100000>
class ParallelDispatcherAbsBench : public ::benchmark::Fixture
{
public:
  using ndarray_type       = SharedArray<type>;
  using VectorRegisterType = typename ndarray_type::VectorRegisterType;

protected:
  void SetUp(::benchmark::State const & /*st*/) override
  {
    a_ = ndarray_type(N);
    for (std::size_t i = 0; i < N; ++i)
    {
      b_[i] = -type(i);
    }
  }

  ndarray_type a_, b_;
};
BENCHMARK_TEMPLATE_F(ParallelDispatcherAbsBench, abs_implementation, double)  // NOLINT
(benchmark::State &st)
{
  // Standard implementation
  for (auto _ : st)
  {
    // Here we use a kernel to compute the same, using an approximation
    a_.in_parallel().Apply([](auto const &x, auto &y) { y = fetch::vectorise::Abs(x); }, b_);
  }
}
