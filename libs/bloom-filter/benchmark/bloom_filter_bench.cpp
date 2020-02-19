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

#include "benchmark/benchmark.h"
#include "bloom_filter/historical_bloom_filter.hpp"
#include "core/byte_array/byte_array.hpp"
#include "crypto/mcl_dkg.hpp"
#include "crypto/sha256.hpp"

using fetch::bloom::HistoricalBloomFilter;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;
using fetch::crypto::SHA256;

ConstByteArray GenerateDigest(uint64_t seed)
{
  SHA256 hasher{};
  hasher.Update(reinterpret_cast<uint8_t const *>(&seed), sizeof(seed));
  auto const v1 = hasher.Final();

  hasher.Reset();
  hasher.Update(v1);
  return {hasher.Final()};
}

void Historical_Bloom_AddHot(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  HistoricalBloomFilter bloom{HistoricalBloomFilter::Mode::NEW_DATABASE, "h-bloom-bench.db",
                              "h-bloom-bench.meta.db", 10000, 1};

  uint64_t  counter{0};
  ByteArray buffer;
  buffer.Resize(sizeof(counter));

  auto *buffer_as_counter = reinterpret_cast<uint64_t *>(buffer.pointer());

  for (auto _ : state)
  {
    *buffer_as_counter = counter++;

    bloom.Add(buffer, 1);
  }
}

void Historical_Bloom_WorstCase(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  HistoricalBloomFilter bloom{HistoricalBloomFilter::Mode::NEW_DATABASE, "h-bloom-bench.db",
                              "h-bloom-bench.meta.db", 1, 1};

  uint64_t  counter{0};
  ByteArray buffer;
  buffer.Resize(sizeof(counter));

  auto *buffer_as_counter = reinterpret_cast<uint64_t *>(buffer.pointer());
  *buffer_as_counter      = ++counter;

  // add a load of pages to the file
  for (uint64_t i = 0; i < 128; ++i)
  {
    bloom.Add(buffer, i);
  }

  // ensure only the last hot page is active in memory
  bloom.TrimCache();

  for (auto _ : state)
  {
    bloom.Match(buffer, 1, 256);
  }
}

void Historical_Bloom_NormalCase(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  HistoricalBloomFilter bloom{HistoricalBloomFilter::Mode::NEW_DATABASE, "h-bloom-bench.db",
                              "h-bloom-bench.meta.db", 128, 1};

  uint64_t  counter{0};
  ByteArray buffer;
  buffer.Resize(sizeof(counter));

  auto *buffer_as_counter = reinterpret_cast<uint64_t *>(buffer.pointer());

  // add a load of pages to the file
  for (uint64_t i = 0; i < 256; ++i)
  {
    *buffer_as_counter = ++counter;
    bloom.Add(buffer, i);
  }

  // ensure only the last hot page is active in memory
  bloom.TrimCache();

  for (auto _ : state)
  {
    bloom.Match(buffer, 1, 256);
  }
}

void Historical_Bloom_Flush(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  auto d1 = GenerateDigest(1);

  for (auto _ : state)
  {
    // create first version of the bloom filter
    state.PauseTiming();
    HistoricalBloomFilter bloom{HistoricalBloomFilter::Mode::NEW_DATABASE, "h-bloom-bench.db",
                                "h-bloom-bench.meta.db", 128, 1};
    bloom.Add(d1, 1);
    state.ResumeTiming();

    bloom.Flush();
  }
}

void Historical_Bloom_Prune(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  auto d1 = GenerateDigest(1);
  auto d2 = GenerateDigest(2);

  for (auto _ : state)
  {
    // create first version of the bloom filter
    state.PauseTiming();
    HistoricalBloomFilter bloom{HistoricalBloomFilter::Mode::NEW_DATABASE, "h-bloom-bench.db",
                                "h-bloom-bench.meta.db", 128, 1};
    bloom.Add(d1, 1);
    state.ResumeTiming();

    bloom.Add(d2, 200);
  }
}

BENCHMARK(Historical_Bloom_AddHot);
// BENCHMARK(Historical_Bloom_WorstCase);
BENCHMARK(Historical_Bloom_NormalCase);
BENCHMARK(Historical_Bloom_Flush);
BENCHMARK(Historical_Bloom_Prune);
