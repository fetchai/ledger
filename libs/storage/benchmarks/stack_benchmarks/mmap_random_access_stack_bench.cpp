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

#include "core/random/lfg.hpp"
#include "storage/mmap_random_access_stack.hpp"

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

using fetch::storage::MMapRandomAccessStack;

class MMapRandomAccessStackBench : public ::benchmark::Fixture
{
protected:
  void SetUp(::benchmark::State const & /*st*/) override
  {
    stack_.New("test_bench.db");
    EXPECT_TRUE(stack_.is_open());
  }

  MMapRandomAccessStack<uint64_t>           stack_;
  fetch::random::LaggedFibonacciGenerator<> lfg_;
};

BENCHMARK_F(MMapRandomAccessStackBench, WritingIntToStack)(benchmark::State &st)  // NOLINT
{
  uint64_t random;
  for (auto _ : st)
  {
    st.PauseTiming();
    random = lfg_();
    st.ResumeTiming();
    stack_.Push(random);
  }
}
