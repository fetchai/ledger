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
#include "storage/cache_line_random_access_stack.hpp"

#include "benchmark/benchmark.h"
#include "gtest/gtest.h"

using fetch::storage::CacheLineRandomAccessStack;

// To calculate the size of cache
static constexpr std::size_t cache_line_size = 13;
template <class T>
struct CachedDataItem
{
  uint64_t                             reads      = 0;
  uint64_t                             writes     = 0;
  uint32_t                             usage_flag = 0;
  std::array<T, 1u << cache_line_size> elements;
};

template <class N>
static void CacheLineRandomAccessStackBench_misshit(benchmark::State &st)
{
  CacheLineRandomAccessStack<N>             stack;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  stack.New("RAS_bench.db");

  EXPECT_TRUE(stack.is_open());
  EXPECT_TRUE(stack.DirectWrite()) << "Expected cache line random access stack to be direct write";

  stack.SetMemoryLimit(std::size_t(1ULL << 22u));
  N        dummy;
  uint64_t line_count = 0;

  // Pushing elements till the cache gets full
  while (line_count * sizeof(CachedDataItem<N>) < std::size_t(1ULL << 22u))
  {
    // Each cache line contains 8192 elements
    for (int i = 0; i < 8192; i++)
    {
      stack.Push(dummy);
    }
    line_count++;
  }
  // Assuming 50% hit rate by generating random numbers double of the orginal count
  uint64_t limit = 8192 * line_count * 2;

  uint64_t random;
  for (auto _ : st)
  {
    st.PauseTiming();
    random = lfg() % limit;
    st.ResumeTiming();
    stack.Set(random, dummy);
  }
}
BENCHMARK_TEMPLATE(CacheLineRandomAccessStackBench_misshit, uint64_t);
BENCHMARK_TEMPLATE(CacheLineRandomAccessStackBench_misshit, double);
BENCHMARK_TEMPLATE(CacheLineRandomAccessStackBench_misshit, int);

template <class N>
static void CacheLineRandomAccessStackBench_miss(benchmark::State &st)
{
  CacheLineRandomAccessStack<N> stack;
  stack.New("RAS_bench.db");

  EXPECT_TRUE(stack.is_open());
  EXPECT_TRUE(stack.DirectWrite()) << "Expected cache line random access stack to be direct write";

  stack.SetMemoryLimit(std::size_t(1ULL << 22u));
  N        dummy{};
  uint64_t line_count = 0;

  // Pushing elements till the cache gets full
  while (line_count * sizeof(CachedDataItem<N>) < std::size_t(1ULL << 22u))
  {
    for (int i = 0; i < 8192; i++)
    {
      stack.Push(dummy);
    }
    line_count++;
  }
  // incrementing index by 8192 to enusre miss
  uint64_t element_count = 8192 * line_count;

  for (auto _ : st)
  {
    st.PauseTiming();
    element_count = (element_count + 8192) % 10000000;
    st.ResumeTiming();
    stack.Set(element_count, dummy);
  }
}
BENCHMARK_TEMPLATE(CacheLineRandomAccessStackBench_miss, uint64_t);
BENCHMARK_TEMPLATE(CacheLineRandomAccessStackBench_miss, double);
BENCHMARK_TEMPLATE(CacheLineRandomAccessStackBench_miss, int);

template <class N>
static void CacheLineRandomAccessStackBench_hit(benchmark::State &st)
{
  CacheLineRandomAccessStack<N>             stack;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  stack.New("RAS_bench.db");

  EXPECT_TRUE(stack.is_open());
  EXPECT_TRUE(stack.DirectWrite()) << "Expected cache line random access stack to be direct write";

  stack.SetMemoryLimit(std::size_t(1ULL << 22u));
  N dummy;

  uint64_t line_count = 0;

  // Pushing elements till the cache gets full
  while (line_count * sizeof(CachedDataItem<N>) < std::size_t(1ULL << 22u))
  {
    for (int i = 0; i < 8192; i++)
    {
      stack.Push(dummy);
    }
    line_count++;
  }

  uint64_t element_count = 8192 * line_count;
  uint64_t random;
  for (auto _ : st)
  {
    st.PauseTiming();
    random = lfg() % element_count;
    st.ResumeTiming();
    stack.Set(random, dummy);
  }
}
BENCHMARK_TEMPLATE(CacheLineRandomAccessStackBench_hit, uint64_t);
BENCHMARK_TEMPLATE(CacheLineRandomAccessStackBench_hit, double);
BENCHMARK_TEMPLATE(CacheLineRandomAccessStackBench_hit, int);
