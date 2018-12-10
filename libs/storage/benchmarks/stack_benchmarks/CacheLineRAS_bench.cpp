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

#include <benchmark/benchmark.h>
#include <gtest/gtest.h>
#include <stack>

#include "core/random/lfg.hpp"
#include "storage/CacheLineRAS.hpp"

using fetch::storage::CacheLineRAS;

template <class N>
static void CacheLineRASBench_misshit(benchmark::State &st)
{

  CacheLineRAS<N>                           stack_;
  fetch::random::LaggedFibonacciGenerator<> lfg_;
  stack_.New("RAS_bench.db");

  EXPECT_TRUE(stack_.is_open());
  EXPECT_TRUE(stack_.DirectWrite() == true)
      << "Expected slightly better random access stack to be direct write";

  stack_.SetMemoryLimit(std::size_t(1ULL << 22));

  for (N i = 0; i < 524288; i++)
  {
    stack_.Push(i);
  }

  uint64_t random;
  N        dummy;
  for (auto _ : st)
  {
    random = lfg_() % 999999;
    stack_.Set(random, dummy);
  }
}
BENCHMARK_TEMPLATE(CacheLineRASBench_misshit, uint64_t);
BENCHMARK_TEMPLATE(CacheLineRASBench_misshit, double);
BENCHMARK_TEMPLATE(CacheLineRASBench_misshit, int);

template <class N>
static void CacheLineRASBench_miss(benchmark::State &st)
{

  CacheLineRAS<N> stack_;
  stack_.New("RAS_bench.db");

  EXPECT_TRUE(stack_.is_open());
  EXPECT_TRUE(stack_.DirectWrite() == true)
      << "Expected slightly better random access stack to be direct write";

  stack_.SetMemoryLimit(std::size_t(1ULL << 22));

  for (N i = 0; i < 524288; i++)
  {
    stack_.Push(i);
  }

  uint64_t random = 524288;
  N        dummy;
  for (auto _ : st)
  {
    random = (random + 8192) % 9999999;
    stack_.Set(random, dummy);
  }
}
BENCHMARK_TEMPLATE(CacheLineRASBench_miss, uint64_t);
BENCHMARK_TEMPLATE(CacheLineRASBench_miss, double);
BENCHMARK_TEMPLATE(CacheLineRASBench_miss, int);

template <class N>
static void CacheLineRASBench_hit(benchmark::State &st)
{

  CacheLineRAS<N>                           stack_;
  fetch::random::LaggedFibonacciGenerator<> lfg_;
  stack_.New("RAS_bench.db");

  EXPECT_TRUE(stack_.is_open());
  EXPECT_TRUE(stack_.DirectWrite() == true)
      << "Expected slightly better random access stack to be direct write";

  stack_.SetMemoryLimit(std::size_t(1ULL << 22));

  for (N i = 0; i < 524288; i++)
  {
    stack_.Push(i);
  }

  uint64_t random;
  N        dummy;
  for (auto _ : st)
  {
    random = lfg_() % 524288;
    stack_.Set(random, dummy);
  }
}
BENCHMARK_TEMPLATE(CacheLineRASBench_hit, uint64_t);
BENCHMARK_TEMPLATE(CacheLineRASBench_hit, double);
BENCHMARK_TEMPLATE(CacheLineRASBench_hit, int);
