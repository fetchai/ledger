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

#include "core/bloom_filter.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/testing/block_generator.hpp"

#include "benchmark/benchmark.h"

#include <memory>

namespace {

using fetch::ledger::MainChain;
using fetch::ledger::testing::BlockGenerator;

using MainChainPtr = std::unique_ptr<MainChain>;
using BlockArray   = std::vector<BlockGenerator::BlockPtr>;

BlockArray GenerateBlocks(benchmark::State const &state)
{
  static constexpr std::size_t NUM_LANES       = 1;
  static constexpr std::size_t NUM_SLICES      = 2;
  static constexpr std::size_t ITERATION_MULTI = 10;

  std::size_t const total_blocks = (ITERATION_MULTI * state.max_iterations) + 1;

  BlockArray array(total_blocks);

  BlockGenerator gen{NUM_LANES, NUM_SLICES};

  // genesis
  array[0] = gen.Generate();

  for (std::size_t i = 1; i < total_blocks; ++i)
  {
    auto const &previous = array[i - 1];

    array[i] = gen.Generate(previous);
  }

  return array;
}

void MainChain_InMemory_AddBlocksSequentially(benchmark::State &state)
{
  auto array = GenerateBlocks(state);

  for (auto _ : state)
  {
    state.PauseTiming();
    auto chain = std::make_unique<MainChain>(std::make_unique<fetch::NullBloomFilter>(),
                                             MainChain::Mode::IN_MEMORY_DB);
    state.ResumeTiming();

    for (std::size_t i = 1; i < array.size(); ++i)
    {
      chain->AddBlock(*array[i]);
    }
  }
}

void MainChain_Persistent_AddBlocksSequentially(benchmark::State &state)
{
  auto array = GenerateBlocks(state);

  for (auto _ : state)
  {
    state.PauseTiming();
    auto chain = std::make_unique<MainChain>(std::make_unique<fetch::NullBloomFilter>(),
                                             MainChain::Mode::CREATE_PERSISTENT_DB);
    state.ResumeTiming();

    for (std::size_t i = 1; i < array.size(); ++i)
    {
      chain->AddBlock(*array[i]);
    }
  }
}

void MainChain_InMemory_AddBlocksOutOfOrder(benchmark::State &state)
{
  auto array = GenerateBlocks(state);

  for (auto _ : state)
  {
    state.PauseTiming();
    auto chain = std::make_unique<MainChain>(std::make_unique<fetch::NullBloomFilter>(),
                                             MainChain::Mode::IN_MEMORY_DB);
    state.ResumeTiming();

    for (std::size_t i = array.size() - 1; i > 0; --i)
    {
      chain->AddBlock(*array[i]);
    }
  }
}

void MainChain_Persistent_AddBlocksOutOfOrder(benchmark::State &state)
{
  auto array = GenerateBlocks(state);

  for (auto _ : state)
  {
    state.PauseTiming();
    auto chain = std::make_unique<MainChain>(std::make_unique<fetch::NullBloomFilter>(),
                                             MainChain::Mode::CREATE_PERSISTENT_DB);
    state.ResumeTiming();

    for (std::size_t i = array.size() - 1; i > 0; --i)
    {
      chain->AddBlock(*array[i]);
    }
  }
}

}  // namespace

BENCHMARK(MainChain_InMemory_AddBlocksSequentially);
BENCHMARK(MainChain_Persistent_AddBlocksSequentially);
BENCHMARK(MainChain_InMemory_AddBlocksOutOfOrder);
BENCHMARK(MainChain_Persistent_AddBlocksOutOfOrder);
