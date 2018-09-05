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

#include "ledger/chain/main_chain.hpp"
#include "core/random/lfg.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "testing/unittest.hpp"
#include <iostream>
#include <list>

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
using miner      = fetch::chain::consensus::DummyMiner;

int main(int argc, char const **argv)
{
  SCENARIO("testing main chain")
  {
    SECTION("building on main chain")
    {
      block_type block;

      MainChain mainChain{};
      mainChain.reset();

      auto genesis = mainChain.HeaviestBlock();

      EXPECT(genesis.body().block_number == 0);

      // Try adding a non-sequential block (prev hash is itself)
      block_type dummy;
      body_type  dummy_body;
      dummy_body.block_number = 1;
      dummy.SetBody(dummy_body);
      dummy.UpdateDigest();
      dummy.body().previous_hash = dummy.hash();

      mainChain.AddBlock(dummy);

      EXPECT(mainChain.HeaviestBlock().hash() == genesis.hash());

      fetch::byte_array::ByteArray prevHash = genesis.hash();

      // Add another 3 blocks in order
      for (std::size_t i = 0; i < 3; ++i)
      {
        fetch::logger.Info("Test: Adding blocks in order");

        // Create another block sequential to previous
        block_type nextBlock;
        body_type  nextBody;
        nextBody.block_number  = 1 + i;
        nextBody.previous_hash = prevHash;

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        mainChain.AddBlock(nextBlock);

        EXPECT(mainChain.HeaviestBlock().hash() == nextBlock.hash());

        prevHash = nextBlock.hash();
      }
    };

    SECTION("Testing for addition of blocks, out of order")
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

      EXPECT(mainChain.HeaviestBlock().hash() == block.hash());

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

      EXPECT(mainChain.HeaviestBlock().hash() == prevHash);
    };

    SECTION("Testing for addition of blocks, with a break")
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

      EXPECT(!(mainChain.HeaviestBlock().hash() == prevHash));
      EXPECT(mainChain.HeaviestBlock().hash() == topHash);
    };

    SECTION("Test mining/proof")
    {
      std::vector<block_type> blocks;
      std::size_t             blockIterations = 10;

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

          miner::Mine(block);

          blocks.push_back(block);
        }

        auto t2 = TimePoint();
        std::cout << "Difficulty: " << diff
                  << ". Block time: " << TimeDifference(t2, t1) / double(blockIterations)
                  << std::endl;
      }

      // Verify blocks
      for (auto &i : blocks)
      {
        if (!i.proof()())
        {
          EXPECT(i.proof()() == 1);
        }
      }
    };

    SECTION("Test mining/proof after serialization")
    {
      std::vector<block_type> blocks;

      for (std::size_t j = 0; j < 10; ++j)
      {
        block_type block;
        body_type  block_body;
        block_body.block_number = j;
        block_body.nonce        = 0;
        block.SetBody(block_body);
        block.UpdateDigest();
        block.proof().SetTarget(8);  // Number of zeroes

        miner::Mine(block);

        blocks.push_back(block);
      }

      bool blockVerified = true;

      // Verify blocks
      for (auto &i : blocks)
      {
        fetch::serializers::ByteArrayBuffer arr;
        arr << i;
        arr.Seek(0);
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

        EXPECT(ToHex(i.hash()) == ToHex(block.hash()));
      }

      EXPECT(blockVerified == true);
    };

    SECTION("Testing time to add blocks sequentially")
    {
      MainChain mainChain{};
      mainChain.reset();

      auto block = mainChain.HeaviestBlock();

      fetch::byte_array::ByteArray prevHash       = block.hash();
      constexpr std::size_t        blocksToCreate = 1000000;
      std::vector<block_type>      blocks(blocksToCreate, block);
      uint64_t                     blockNumber = block.body().block_number++;

      {
        auto t1 = TimePoint();

        // Precreate since UpdateDigest not part of test
        for (std::size_t i = 0; i < blocksToCreate; ++i)
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

      for (std::size_t i = 0; i < blocksToCreate; ++i)
      {
        mainChain.AddBlock(blocks[i]);
      }

      auto t2 = TimePoint();
      std::cout << "Blocks: " << blocksToCreate << ". Time: " << TimeDifference(t2, t1)
                << std::endl;

      EXPECT(mainChain.HeaviestBlock().hash() == prevHash);
    };

    SECTION("Testing time to add blocks out of order")
    {
      MainChain mainChain{};
      mainChain.reset();

      auto block = mainChain.HeaviestBlock();

      fetch::byte_array::ByteArray              prevHash       = block.hash();
      constexpr std::size_t                     blocksToCreate = 1000000;
      std::vector<block_type>                   blocks(blocksToCreate, block);
      std::map<std::size_t, std::size_t>        randomIndexes;
      uint64_t                                  blockNumber = block.body().block_number++;
      fetch::random::LaggedFibonacciGenerator<> lfg;

      {
        auto t1 = TimePoint();
        // Precreate since UpdateDigest not part of test
        for (std::size_t i = 0; i < blocksToCreate; ++i)
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

        randomIndexes = GetRandomIndexes(blocksToCreate);

        auto t2 = TimePoint();
        std::cout << "Setup time: " << TimeDifference(t2, t1) << std::endl;
      }

      auto t1 = TimePoint();

      for (auto &i : randomIndexes)
      {
        mainChain.AddBlock(blocks[i.second]);
      }

      auto t2 = TimePoint();
      std::cout << "Blocks: " << blocksToCreate << ". Time: " << TimeDifference(t2, t1)
                << std::endl;

      // Last block in vector still heaviest block for main chain
      EXPECT(mainChain.HeaviestBlock().totalWeight() == blocksToCreate + 1);
      EXPECT(ToHex(mainChain.HeaviestBlock().hash()) == ToHex(prevHash));
    };

    SECTION("Testing time to add blocks sequentially - with file storage")
    {
      MainChain mainChain{0};
      mainChain.reset();

      auto block = mainChain.HeaviestBlock();

      fetch::byte_array::ByteArray prevHash       = block.hash();
      constexpr std::size_t        blocksToCreate = 1000000;
      std::vector<block_type>      blocks(blocksToCreate, block);
      uint64_t                     blockNumber = block.body().block_number++;

      {
        auto t1 = TimePoint();

        // Precreate since UpdateDigest not part of test
        for (std::size_t i = 0; i < blocksToCreate; ++i)
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

      for (std::size_t i = 0; i < blocksToCreate; ++i)
      {
        mainChain.AddBlock(blocks[i]);
      }

      auto t2 = TimePoint();
      std::cout << "Blocks: " << blocksToCreate << ". Time: " << TimeDifference(t2, t1)
                << std::endl;

      EXPECT(mainChain.HeaviestBlock().hash() == prevHash);
    };

    SECTION("Testing time to add blocks out of order - with file storage")
    {
      MainChain mainChain{0};
      mainChain.reset();

      auto block = mainChain.HeaviestBlock();

      fetch::byte_array::ByteArray              prevHash       = block.hash();
      constexpr std::size_t                     blocksToCreate = 1000000;
      std::vector<block_type>                   blocks(blocksToCreate, block);
      std::map<std::size_t, std::size_t>        randomIndexes;
      uint64_t                                  blockNumber = block.body().block_number++;
      fetch::random::LaggedFibonacciGenerator<> lfg;

      {
        auto t1 = TimePoint();

        // Precreate since UpdateDigest not part of test
        for (std::size_t i = 0; i < blocksToCreate; ++i)
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

        randomIndexes = GetRandomIndexes(blocksToCreate);

        auto t2 = TimePoint();
        std::cout << "Setup time: " << TimeDifference(t2, t1) << std::endl;
      }

      auto t1 = TimePoint();

      for (auto &i : randomIndexes)
      {
        mainChain.AddBlock(blocks[i.second]);
      }

      auto t2 = TimePoint();
      std::cout << "Blocks: " << blocksToCreate << ". Time: " << TimeDifference(t2, t1)
                << std::endl;

      // Last block in vector still heaviest block for main chain
      EXPECT(mainChain.HeaviestBlock().totalWeight() == blocksToCreate + 1);
      EXPECT(ToHex(mainChain.HeaviestBlock().hash()) == ToHex(prevHash));
    };
  };

  return 0;
}
