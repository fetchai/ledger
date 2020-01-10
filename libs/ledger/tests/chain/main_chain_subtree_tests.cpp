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

#include "bloom_filter/bloom_filter.hpp"
#include "core/byte_array/encoders.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/testing/block_generator.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <vector>

namespace {

using fetch::ledger::MainChain;
using fetch::ledger::BlockHash;
using fetch::ledger::BlockPtr;
using fetch::ledger::BlockStatus;
using fetch::ledger::Blocks;
using fetch::ledger::testing::BlockGenerator;

using MainChainPtr = std::unique_ptr<MainChain>;
using BlockPtr     = BlockGenerator::BlockPtr;

class MainChainSubTreeTests : public ::testing::Test
{
protected:
  static constexpr std::size_t NUM_LANES  = 1;
  static constexpr std::size_t NUM_SLICES = 1;

  void SetUp() override
  {
    fetch::crypto::mcl::details::MCLInitialiser();
    block_generator_.Reset();

    chain_ = std::make_unique<MainChain>(MainChain::Mode::IN_MEMORY_DB);
  }

  BlockGenerator block_generator_{NUM_LANES, NUM_SLICES};
  MainChainPtr   chain_;

public:
  Blocks GetAncestorInLimit(MainChain::BehaviourWhenLimit behaviour, BlockPtr const &b1,
                            BlockPtr const &b3)
  {
    constexpr uint64_t subchain_length_limit = 2;

    Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, b3->hash, b1->hash, subchain_length_limit,
                                                behaviour));
    EXPECT_EQ(subchain_length_limit, blocks.size());

    return blocks;
  }
};

Blocks Extract(Blocks const &input, std::initializer_list<std::size_t> indexes)
{
  Blocks output;
  output.reserve(indexes.size());

  for (auto const &index : indexes)
  {
    output.push_back(input.at(index));
  }

  return output;
}

bool AreEqual(Blocks const &actual, Blocks const &expected)
{
  return std::equal(actual.begin(), actual.end(), expected.begin(), expected.end(),
                    [](auto const &a, auto const &b) { return a->hash == b->hash; });
}

TEST_F(MainChainSubTreeTests, CheckSimpleTree)
{
  //
  //             ┌────┐
  //         ┌──▶│ B1 │
  // ┌────┐  │   └────┘
  // │ GN │──┤
  // └────┘  │   ┌────┐
  //         └──▶│ B2 │
  //             └────┘
  //
  auto genesis = block_generator_();
  auto b1      = block_generator_(genesis);
  auto b2      = block_generator_(genesis);

  // add the blocks to the main chain
  ASSERT_EQ(BlockStatus::DUPLICATE, chain_->AddBlock(*genesis));  // genesis is always present
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*b1));
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*b2));

  BlockPtr blk = chain_->GetBlock(b2->hash);
  ASSERT_TRUE(blk);
  EXPECT_FALSE(blk->is_loose);
  EXPECT_EQ(blk->hash, b2->hash);
  EXPECT_EQ(blk->previous_hash, b2->previous_hash);

  blk = chain_->GetBlock(b1->hash);
  ASSERT_TRUE(blk);
  EXPECT_FALSE(blk->is_loose);
  EXPECT_EQ(blk->hash, b1->hash);
  EXPECT_EQ(blk->previous_hash, b1->previous_hash);

  blk = chain_->GetBlock(genesis->hash);
  ASSERT_TRUE(blk);
  EXPECT_FALSE(blk->is_loose);
  EXPECT_EQ(blk->hash, genesis->hash);
  EXPECT_EQ(blk->previous_hash, genesis->previous_hash);
}

TEST_F(MainChainSubTreeTests, CheckCommonSubTree)
{
  // Simple tree structure
  //
  //             ┌────┐
  //         ┌──▶│ B1 │
  // ┌────┐  │   └────┘
  // │ GN │──┤
  // └────┘  │   ┌────┐     ┌────┐
  //         └──▶│ B2 │────▶│ B3 │
  //             └────┘     └────┘
  //
  auto genesis = block_generator_();
  auto b1      = block_generator_(genesis);
  auto b2      = block_generator_(genesis);
  auto b3      = block_generator_(b2);

  // add the blocks to the main chain
  for (auto const &block : {b1, b2, b3})
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block));
  }

  Blocks blocks;
  EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, b3->hash, b1->hash));
  ASSERT_EQ(3, blocks.size());
  EXPECT_EQ(b3->hash, blocks[0]->hash);
  EXPECT_EQ(b2->hash, blocks[1]->hash);
  EXPECT_EQ(genesis->hash, blocks[2]->hash);
}

TEST_F(MainChainSubTreeTests, CheckCommonSubTree2)
{
  // Simple tree structure
  //
  //             ┌────┐
  //         ┌──▶│ B1 │
  // ┌────┐  │   └────┘
  // │ GN │──┤
  // └────┘  │   ┌────┐     ┌────┐
  //         └──▶│ B2 │────▶│ B3 │
  //             └────┘     └────┘
  //
  auto genesis = block_generator_();
  auto b1      = block_generator_(genesis);
  auto b2      = block_generator_(genesis);
  auto b3      = block_generator_(b2);

  // add the blocks to the main chain
  for (auto const &block : {b1, b2, b3})
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block));
  }

  Blocks blocks;
  EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, b1->hash, b3->hash));

  ASSERT_EQ(2, blocks.size());
  EXPECT_EQ(b1->hash, blocks[0]->hash);
  EXPECT_EQ(genesis->hash, blocks[1]->hash);
}

TEST_F(MainChainSubTreeTests, CheckLooseBlocks)
{
  // Simple tree structure
  //
  //             ┌────┐
  //         ┌──▶│ B1 │
  // ┌────┐  │   └────┘
  // │ GN │──┤
  // └────┘      ┌────┐     ┌────┐
  //         └ ─▶│ B2 │─ ─ ▶│ B3 │
  //             └────┘     └────┘
  //
  auto genesis = block_generator_();
  auto b1      = block_generator_(genesis);
  auto b2      = block_generator_(genesis);  // (missing block)
  auto b3      = block_generator_(b2);

  // add the blocks to the main chain
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*b1));
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*b3));

  Blocks blocks;
  EXPECT_FALSE(chain_->GetPathToCommonAncestor(blocks, b3->hash, b1->hash));

  // missing block turns up
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*b2));

  // ensure that the sub tree can now be located
  EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, b3->hash, b1->hash));
  ASSERT_EQ(3, blocks.size());
  EXPECT_EQ(b3->hash, blocks[0]->hash);
  EXPECT_EQ(b2->hash, blocks[1]->hash);
  EXPECT_EQ(genesis->hash, blocks[2]->hash);
}

TEST_F(MainChainSubTreeTests, ComplicatedSubTrees)
{
  // Complicates graph structure
  //                                                           ┌────┐
  //                                                        ┌─▶│ B9 │
  //                                                ┌────┐  │  └────┘
  //                                            ┌──▶│ B5 │──┤
  //                                            │   └────┘  │  ┌────┐
  //                                            │           └─▶│B10 │
  //                                   ┌────┐   │              └────┘
  //                                ┌─▶│ B3 │───┤
  //                                │  └────┘   │              ┌────┐
  //                                │           │           ┌─▶│B11 │
  //                                │           │   ┌────┐  │  └────┘
  //                                │           └──▶│ B6 │──┤
  //                                │               └────┘  │  ┌────┐
  //                                │                       └─▶│B12 │
  // ┌────┐      ┌────┐     ┌────┐  │                          └────┘
  // │ GN │ ────▶│ B1 │────▶│ B2 │──┤
  // └────┘      └────┘     └────┘  │                          ┌────┐
  //                                │                       ┌─▶│B13 │
  //                                │               ┌────┐  │  └────┘
  //                                │           ┌──▶│ B7 │──┤
  //                                │           │   └────┘  │  ┌────┐
  //                                │           │           └─▶│B14 │
  //                                │  ┌────┐   │              └────┘
  //                                └─▶│ B4 │───┤
  //                                   └────┘   │              ┌────┐
  //                                            │           ┌─▶│B15 │
  //                                            │   ┌────┐  │  └────┘
  //                                            └──▶│ B8 │──┤
  //                                                └────┘  │  ┌────┐
  //                                                        └─▶│B16 │
  //                                                           └────┘
  //
  Blocks chain(17);
  chain[0] = block_generator_();
  chain[1] = block_generator_(chain[0]);
  chain[2] = block_generator_(chain[1]);
  for (std::size_t source = 2, dest = 3; dest < chain.size(); ++source)
  {
    chain[dest++] = block_generator_(chain[source]);
    chain[dest++] = block_generator_(chain[source]);
  }

  // add all the blocks to the chain
  for (std::size_t i = 1; i < chain.size(); ++i)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain[i]));
  }

  // B13 vs. B12
  {
    Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, chain[13]->hash, chain[12]->hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {13, 7, 4, 2})));
  }

  // B16 vs. B15
  {
    Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, chain[16]->hash, chain[15]->hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {16, 8})));
  }

  // B16 vs. B14
  {
    Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, chain[16]->hash, chain[14]->hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {16, 8, 4})));
  }

  // B16 vs. B2
  {
    Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, chain[16]->hash, chain[2]->hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {16, 8, 4, 2})));
  }

  // B1 vs. B16
  {
    Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, chain[1]->hash, chain[16]->hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {1})));
  }

  // B4 vs. B11
  {
    Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, chain[4]->hash, chain[11]->hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {4, 2})));
  }
}

TEST_F(MainChainSubTreeTests,
       Check_Common_Ancestor_With_Limit_Exceeded_Yields_Path_Including_Ancestor)
{
  // Simple tree structure
  //
  //             ┌────┐
  //         ┌──▶│ B1 │
  // ┌────┐  │   └────┘
  // │ GN │──┤
  // └────┘  │   ┌────┐     ┌────┐
  //         └──▶│ B2 │────▶│ B3 │
  //             └────┘     └────┘
  //
  auto genesis = block_generator_();
  auto b1      = block_generator_(genesis);
  auto b2      = block_generator_(genesis);
  auto b3      = block_generator_(b2);

  // add the blocks to the main chain
  for (auto const &block : {b1, b2, b3})
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block));
  }

  auto blocks = GetAncestorInLimit(MainChain::BehaviourWhenLimit::RETURN_LEAST_RECENT, b1, b3);

  EXPECT_EQ(b2->hash, blocks[0]->hash);
  EXPECT_EQ(genesis->hash, blocks[1]->hash);
}

TEST_F(MainChainSubTreeTests,
       Check_Common_Ancestor_With_Limit_Exceeded_Yields_Path_Not_Including_Ancestor)
{
  // Simple tree structure
  //
  //             ┌────┐
  //         ┌──▶│ B1 │
  // ┌────┐  │   └────┘
  // │ GN │──┤
  // └────┘  │   ┌────┐     ┌────┐
  //         └──▶│ B2 │────▶│ B3 │
  //             └────┘     └────┘
  //
  auto genesis = block_generator_();
  auto b1      = block_generator_(genesis);
  auto b2      = block_generator_(genesis);
  auto b3      = block_generator_(b2);

  // add the blocks to the main chain
  for (auto const &block : {b1, b2, b3})
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block));
  }

  auto blocks = GetAncestorInLimit(MainChain::BehaviourWhenLimit::RETURN_MOST_RECENT, b1, b3);

  EXPECT_EQ(b3->hash, blocks[0]->hash);
  EXPECT_EQ(b2->hash, blocks[1]->hash);
}

}  // namespace
