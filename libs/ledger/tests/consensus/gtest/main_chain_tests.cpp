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

#include <gtest/gtest.h>
#include <iostream>
#include <list>

#include "core/random/lfg.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "ledger/chain/main_chain.hpp"

static constexpr char const *LOGGING_NAME = "MainChainTests";

using namespace fetch::chain;
using namespace fetch::byte_array;

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

using block_type = MainChain::BlockType;
using body_type  = MainChain::BlockType::body_type;

static constexpr std::size_t NUM_BLOCKS = 1000;

TEST(ledger_main_chain_gtest, building_on_main_chain)
{
  block_type block;

  MainChain mainChain{};
  mainChain.reset();

  auto genesis = mainChain.HeaviestBlock();

  EXPECT_EQ(genesis.body().block_number, 0);

  fetch::byte_array::ByteArray prevHash = genesis.hash();
  fetch::byte_array::ByteArray currHash = genesis.hash();

  // Add another 3 blocks in order
  for (std::size_t i = 0; i < 3; ++i)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Test: Adding blocks in order");

    // Create another block sequential to previous
    block_type nextBlock;
    body_type  nextBody;
    nextBody.block_number  = 1 + i;
    nextBody.previous_hash = prevHash;

    nextBlock.SetBody(nextBody);
    nextBlock.UpdateDigest();

    mainChain.AddBlock(nextBlock);

    EXPECT_EQ(mainChain.HeaviestBlock().hash(), nextBlock.hash());

    prevHash = nextBlock.hash();
    currHash = nextBlock.hash();
  }

  // Try adding a non-sequential block (prev hash is itself)
  block_type dummy;
  body_type  dummy_body;
  dummy_body.block_number = 1;
  dummy.SetBody(dummy_body);
  dummy.UpdateDigest();
  dummy.body().previous_hash = genesis.hash();

  mainChain.AddBlock(dummy);

  // check that the heaviest block has not changed
  EXPECT_EQ(mainChain.HeaviestBlock().hash(), currHash);
}

TEST(ledger_main_chain_gtest, addition_of_blocks_out_of_order)
{
  MainChain mainChain{};
  mainChain.reset();

  auto block = mainChain.HeaviestBlock();

  // Try adding a non-sequential block (prev hash is itself)
  block_type dummy;
  body_type  dummy_body;
  dummy_body.block_number = 2;
  dummy.SetBody(dummy_body);
  dummy.UpdateDigest();
  dummy.body().previous_hash = dummy.hash();

  mainChain.AddBlock(dummy);

  EXPECT_EQ(mainChain.HeaviestBlock().hash(), block.hash());

  fetch::byte_array::ByteArray prevHash = block.hash();
  std::vector<block_type>      blocks;

  // Add another 3 blocks in order
  for (std::size_t i = 0; i < 3; ++i)
  {
    // Create another block sequential to previous
    block_type nextBlock;
    body_type  nextBody;
    nextBody.block_number  = 1 + i;
    nextBody.previous_hash = prevHash;

    nextBlock.SetBody(nextBody);
    nextBlock.UpdateDigest();

    blocks.push_back(nextBlock);

    prevHash = nextBlock.hash();
  }

  for (auto i : blocks)
  {
    mainChain.AddBlock(i);
  }

  EXPECT_EQ(mainChain.HeaviestBlock().hash(), prevHash);
}

TEST(ledger_main_chain_gtest, addition_of_blocks_with_a_break)
{
  MainChain mainChain{};
  mainChain.reset();

  auto block = mainChain.HeaviestBlock();

  fetch::byte_array::ByteArray prevHash = block.hash();
  fetch::byte_array::ByteArray topHash  = block.hash();

  // Add another N blocks in order
  for (std::size_t i = block.body().block_number + 1; i < 15; ++i)
  {
    // Create another block sequential to previous
    block_type nextBlock;
    body_type  nextBody;
    nextBody.block_number  = i;
    nextBody.previous_hash = prevHash;

    nextBlock.SetBody(nextBody);
    nextBlock.UpdateDigest();

    if (i != 7)
    {
      mainChain.AddBlock(block);
    }
    else
    {
      topHash = block.hash();
    }
    prevHash = nextBlock.hash();
  }

  EXPECT_NE(mainChain.HeaviestBlock().hash(), prevHash);
  EXPECT_EQ(mainChain.HeaviestBlock().hash(), topHash);
}

TEST(ledger_main_chain_gtest, Test_mining_proof)
{
  std::vector<block_type>             blocks;
  std::size_t                         blockIterations = 10;
  fetch::chain::consensus::DummyMiner miner;

  for (std::size_t diff = 1; diff < 16; diff <<= 1)
  {
    auto t1 = TimePoint();

    for (std::size_t j = 0; j < blockIterations; ++j)
    {
      block_type block;
      body_type  block_body;
      block_body.block_number = j;
      block_body.nonce        = 0;
      block.SetBody(block_body);
      block.UpdateDigest();
      block.proof().SetTarget(diff);  // Number of zeroes

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
    if (!i.proof()())
    {
      EXPECT_EQ(i.proof()(), 1);
    }
  }
}

TEST(ledger_main_chain_gtest, Test_mining_proof_after_serialization)
{
  std::vector<block_type>             blocks;
  fetch::chain::consensus::DummyMiner miner;

  for (std::size_t j = 0; j < 10; ++j)
  {
    block_type block;
    body_type  block_body;
    block_body.block_number = j;
    block_body.nonce        = 0;
    block.SetBody(block_body);
    block.UpdateDigest();
    block.proof().SetTarget(8);  // Number of zeroes

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
    block_type block;
    arr >> block;

    block.UpdateDigest();  // digest and target not serialized due to trust
                           // issues
    block.proof().SetTarget(8);

    if (!block.proof()())
    {
      std::cout << "block not verified" << std::endl;
      blockVerified = false;
    }

    EXPECT_EQ(ToHex(i.hash()), ToHex(block.hash()));
  }

  EXPECT_EQ(blockVerified, true);
}

TEST(ledger_main_chain_gtest, Testing_time_to_add_blocks_sequentially)
{
  MainChain mainChain{};
  mainChain.reset();

  auto block = mainChain.HeaviestBlock();

  fetch::byte_array::ByteArray prevHash = block.hash();

  std::vector<block_type> blocks(NUM_BLOCKS, block);
  uint64_t                blockNumber = block.body().block_number++;

  {
    auto t1 = TimePoint();

    // Precreate since UpdateDigest not part of test
    for (std::size_t i = 0; i < NUM_BLOCKS; ++i)
    {
      // Create another block sequential to previous
      block_type nextBlock;
      body_type  nextBody;
      nextBody.block_number  = blockNumber++;
      nextBody.previous_hash = prevHash;

      nextBlock.SetBody(nextBody);
      nextBlock.UpdateDigest();

      blocks[i] = nextBlock;

      prevHash = nextBlock.hash();
    }

    auto t2 = TimePoint();
    std::cout << "Setup time: " << TimeDifference(t2, t1) << std::endl;
  }

  auto t1 = TimePoint();

  for (std::size_t i = 0; i < NUM_BLOCKS; ++i)
  {
    mainChain.AddBlock(blocks[i]);
  }

  auto t2 = TimePoint();
  std::cout << "Blocks: " << NUM_BLOCKS << ". Time: " << TimeDifference(t2, t1) << std::endl;

  EXPECT_EQ(mainChain.HeaviestBlock().hash(), prevHash);
}

TEST(ledger_main_chain_gtest, Testing_time_to_add_blocks_out_of_order)
{
  MainChain mainChain{};
  mainChain.reset();

  auto block = mainChain.HeaviestBlock();

  fetch::byte_array::ByteArray              prevHash = block.hash();
  std::vector<block_type>                   blocks(NUM_BLOCKS, block);
  std::map<std::size_t, std::size_t>        randomIndexes;
  uint64_t                                  blockNumber = block.body().block_number++;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  {
    auto t1 = TimePoint();
    // Precreate since UpdateDigest not part of test
    for (std::size_t i = 0; i < NUM_BLOCKS; ++i)
    {
      // Create another block sequential to previous
      block_type nextBlock;
      body_type  nextBody;
      nextBody.block_number  = blockNumber++;
      nextBody.previous_hash = prevHash;

      nextBlock.SetBody(nextBody);
      nextBlock.UpdateDigest();

      blocks[i] = nextBlock;

      prevHash = nextBlock.hash();
    }

    randomIndexes = GetRandomIndexes(NUM_BLOCKS);

    auto t2 = TimePoint();
    std::cout << "Setup time: " << TimeDifference(t2, t1) << std::endl;
  }

  auto t1 = TimePoint();

  for (auto &i : randomIndexes)
  {
    mainChain.AddBlock(blocks[i.second]);
  }

  auto t2 = TimePoint();
  std::cout << "Blocks: " << NUM_BLOCKS << ". Time: " << TimeDifference(t2, t1) << std::endl;

  // Last block in vector still heaviest block for main chain
  EXPECT_EQ(mainChain.HeaviestBlock().totalWeight(), NUM_BLOCKS + 1);
  EXPECT_EQ(ToHex(mainChain.HeaviestBlock().hash()), ToHex(prevHash));
}

TEST(ledger_main_chain_gtest, Testing_time_to_add_blocks_sequentially_with_file_storage)
{
  MainChain mainChain{0};
  mainChain.reset();

  auto block = mainChain.HeaviestBlock();

  fetch::byte_array::ByteArray prevHash = block.hash();
  std::vector<block_type>      blocks(NUM_BLOCKS, block);
  uint64_t                     blockNumber = block.body().block_number++;

  {
    auto t1 = TimePoint();

    // Precreate since UpdateDigest not part of test
    for (std::size_t i = 0; i < NUM_BLOCKS; ++i)
    {
      // Create another block sequential to previous
      block_type nextBlock;
      body_type  nextBody;
      nextBody.block_number  = blockNumber++;
      nextBody.previous_hash = prevHash;

      nextBlock.SetBody(nextBody);
      nextBlock.UpdateDigest();

      blocks[i] = nextBlock;

      prevHash = nextBlock.hash();
    }

    auto t2 = TimePoint();
    std::cout << "Setup time: " << TimeDifference(t2, t1) << std::endl;
  }

  auto t1 = TimePoint();

  for (std::size_t i = 0; i < NUM_BLOCKS; ++i)
  {
    mainChain.AddBlock(blocks[i]);
  }

  auto t2 = TimePoint();
  std::cout << "Blocks: " << NUM_BLOCKS << ". Time: " << TimeDifference(t2, t1) << std::endl;

  EXPECT_EQ(mainChain.HeaviestBlock().hash(), prevHash);
}

TEST(ledger_main_chain_gtest, Testing_time_to_add_blocks_out_of_order_with_file_storage)
{
  MainChain mainChain{0};
  mainChain.reset();

  auto block = mainChain.HeaviestBlock();

  fetch::byte_array::ByteArray              prevHash = block.hash();
  std::vector<block_type>                   blocks(NUM_BLOCKS, block);
  std::map<std::size_t, std::size_t>        randomIndexes;
  uint64_t                                  blockNumber = block.body().block_number++;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  {
    auto t1 = TimePoint();

    // Precreate since UpdateDigest not part of test
    for (std::size_t i = 0; i < NUM_BLOCKS; ++i)
    {
      // Create another block sequential to previous
      block_type nextBlock;
      body_type  nextBody;
      nextBody.block_number  = blockNumber++;
      nextBody.previous_hash = prevHash;

      nextBlock.SetBody(nextBody);
      nextBlock.UpdateDigest();

      blocks[i] = nextBlock;

      prevHash = nextBlock.hash();
    }

    randomIndexes = GetRandomIndexes(NUM_BLOCKS);

    auto t2 = TimePoint();
    std::cout << "Setup time: " << TimeDifference(t2, t1) << std::endl;
  }

  auto t1 = TimePoint();

  for (auto &i : randomIndexes)
  {
    mainChain.AddBlock(blocks[i.second]);
  }

  auto t2 = TimePoint();
  std::cout << "Blocks: " << NUM_BLOCKS << ". Time: " << TimeDifference(t2, t1) << std::endl;

  // Last block in vector still heaviest block for main chain
  EXPECT_EQ(mainChain.HeaviestBlock().totalWeight(), NUM_BLOCKS + 1);
  EXPECT_EQ(ToHex(mainChain.HeaviestBlock().hash()), ToHex(prevHash));
}
