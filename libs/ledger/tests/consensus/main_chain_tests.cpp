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

#include "core/random/lfg.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "ledger/chain/main_chain.hpp"

#include "gtest/gtest.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <vector>

using fetch::ledger::consensus::DummyMiner;
using fetch::ledger::Block;
using fetch::ledger::Address;

using Blocks = std::vector<Block>;
using Body   = Block::Body;

// TODO(issue 33): get these from helper_functions when it's sorted
// Time related functionality
using time_point = std::chrono::high_resolution_clock::time_point;

time_point TimePoint()
{
  return std::chrono::high_resolution_clock::now();
}

double TimeDifference(time_point t1, time_point t2)
{
  // If t1 before t2
  if (t1 < t2)
  {
    return std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count();
  }
  return std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t2).count();
}

std::map<std::size_t, std::size_t> GetRandomIndexes(std::size_t size)
{
  static fetch::random::LaggedFibonacciGenerator<> lfg;
  std::map<std::size_t, std::size_t>               ret;

  for (std::size_t i = 0; i < size; ++i)
  {
    uint64_t indexRnd = i | (lfg() & 0xFFFFFFFF00000000);
    ret[indexRnd]     = i;
  }

  return ret;
}

TEST(ledger_main_chain_gtest, Test_mining_proof)
{
  Blocks      blocks;
  std::size_t blockIterations = 10;
  DummyMiner  miner;

  for (std::size_t diff = 1; diff < 16; diff <<= 1)
  {
    auto t1 = TimePoint();

    for (std::size_t j = 0; j < blockIterations; ++j)
    {
      Block block;
      block.body.block_number = j;
      block.body.miner        = Address{Address::RawAddress{}};
      block.nonce             = 0;
      block.UpdateDigest();
      block.proof.SetTarget(diff);  // Number of zeroes

      miner.Mine(block);

      blocks.push_back(block);
    }

    auto t2 = TimePoint();
    std::cout << "Difficulty: " << diff
              << ". Block time: " << TimeDifference(t2, t1) / double(blockIterations) << std::endl;
  }

  // Verify blocks
  for (auto &i : blocks)
  {
    if (!i.proof())
    {
      EXPECT_EQ(i.proof(), 1);
    }
  }
}

TEST(ledger_main_chain_gtest, Test_mining_proof_after_serialization)
{
  Blocks     blocks;
  DummyMiner miner;

  for (std::size_t j = 0; j < 10; ++j)
  {
    Block block;
    block.body.block_number = j;
    block.body.miner        = Address{Address::RawAddress{}};
    block.nonce             = 0;
    block.UpdateDigest();
    block.proof.SetTarget(8);  // Number of zeroes

    miner.Mine(block);

    blocks.push_back(block);
  }

  bool blockVerified = true;

  // Verify blocks
  for (auto &i : blocks)
  {
    fetch::serializers::ByteArrayBuffer arr;
    arr << i;
    arr.seek(0);
    Block block;
    arr >> block;

    block.UpdateDigest();  // digest and target not serialized due to trust
                           // issues
    block.proof.SetTarget(8);

    if (!block.proof())
    {
      std::cout << "block not verified" << std::endl;
      blockVerified = false;
    }

    EXPECT_EQ(ToHex(i.body.hash), ToHex(block.body.hash));
  }

  EXPECT_EQ(blockVerified, true);
}
