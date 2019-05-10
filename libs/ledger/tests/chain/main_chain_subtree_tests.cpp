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

#include "core/byte_array/encoders.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/testing/block_generator.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <memory>

namespace {

using fetch::ledger::MainChain;
using fetch::ledger::BlockStatus;
using fetch::byte_array::ToBase64;  // NOLINT - needed for debug messages
using fetch::ledger::testing::BlockGenerator;

using MainChainPtr    = std::unique_ptr<MainChain>;
using BlockPtr        = MainChain::BlockPtr;
using MutableBlockPtr = BlockGenerator::BlockPtr;
using Blocks          = std::vector<MutableBlockPtr>;

class MainChainSubTreeTests : public ::testing::Test
{
protected:
  static constexpr std::size_t NUM_LANES  = 1;
  static constexpr std::size_t NUM_SLICES = 1;

  void SetUp() override
  {
    block_generator_.Reset();

    chain_ = std::make_unique<MainChain>(MainChain::Mode::IN_MEMORY_DB);
  }

  void TearDown() override
  {
    chain_.reset();
  }

  BlockGenerator block_generator_{NUM_LANES, NUM_SLICES};
  MainChainPtr   chain_;

public:

MainChain::Blocks GetAncestorInLimit(MainChain::BehaviourWhenLimit behaviour, MainChain::BlockPtr b1, MainChain::BlockPtr b3)
{
  constexpr uint64_t subchain_length_limit = 2;

  MainChain::Blocks blocks;
  EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, b3->body.hash, b1->body.hash, subchain_length_limit, behaviour));
  EXPECT_EQ(subchain_length_limit, blocks.size());

  return blocks;
}
};

static Blocks Extract(Blocks const &input, std::initializer_list<std::size_t> const &indexes)
{
  Blocks output;
  output.reserve(indexes.size());

  for (auto const &index : indexes)
  {
    output.push_back(input.at(index));
  }

  return output;
}

static bool AreEqual(MainChain::Blocks const &actual, Blocks const &expected)
{
  bool equal{false};

  if (actual.size() == expected.size())
  {
    equal = std::equal(actual.begin(), actual.end(), expected.begin(), expected.end(),
                       [](auto const &a, auto const &b) { return a->body.hash == b->body.hash; });
  }

  return equal;
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

  BlockPtr blk = chain_->GetBlock(b2->body.hash);
  ASSERT_TRUE(blk);
  EXPECT_FALSE(blk->is_loose);
  EXPECT_EQ(blk->body.hash, b2->body.hash);
  EXPECT_EQ(blk->body.previous_hash, b2->body.previous_hash);

  blk = chain_->GetBlock(b1->body.hash);
  ASSERT_TRUE(blk);
  EXPECT_FALSE(blk->is_loose);
  EXPECT_EQ(blk->body.hash, b1->body.hash);
  EXPECT_EQ(blk->body.previous_hash, b1->body.previous_hash);

  blk = chain_->GetBlock(genesis->body.hash);
  ASSERT_TRUE(blk);
  EXPECT_FALSE(blk->is_loose);
  EXPECT_EQ(blk->body.hash, genesis->body.hash);
  EXPECT_EQ(blk->body.previous_hash, genesis->body.previous_hash);
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
  for (auto const &block : Blocks{b1, b2, b3})
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block));
  }

  MainChain::Blocks blocks;
  EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, b3->body.hash, b1->body.hash));
  ASSERT_EQ(3, blocks.size());
  EXPECT_EQ(b3->body.hash, blocks[0]->body.hash);
  EXPECT_EQ(b2->body.hash, blocks[1]->body.hash);
  EXPECT_EQ(genesis->body.hash, blocks[2]->body.hash);
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
  for (auto const &block : Blocks{b1, b2, b3})
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block));
  }

  MainChain::Blocks blocks;
  EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, b1->body.hash, b3->body.hash));

  ASSERT_EQ(2, blocks.size());
  EXPECT_EQ(b1->body.hash, blocks[0]->body.hash);
  EXPECT_EQ(genesis->body.hash, blocks[1]->body.hash);
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

  MainChain::Blocks blocks;
  EXPECT_FALSE(chain_->GetPathToCommonAncestor(blocks, b3->body.hash, b1->body.hash));

  // missing block turns up
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*b2));

  // ensure that the sub tree can now be located
  EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, b3->body.hash, b1->body.hash));
  ASSERT_EQ(3, blocks.size());
  EXPECT_EQ(b3->body.hash, blocks[0]->body.hash);
  EXPECT_EQ(b2->body.hash, blocks[1]->body.hash);
  EXPECT_EQ(genesis->body.hash, blocks[2]->body.hash);
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
  chain[0]  = block_generator_();
  chain[1]  = block_generator_(chain[0]);
  chain[2]  = block_generator_(chain[1]);
  chain[3]  = block_generator_(chain[2]);
  chain[4]  = block_generator_(chain[2]);
  chain[5]  = block_generator_(chain[3]);
  chain[6]  = block_generator_(chain[3]);
  chain[7]  = block_generator_(chain[4]);
  chain[8]  = block_generator_(chain[4]);
  chain[9]  = block_generator_(chain[5]);
  chain[10] = block_generator_(chain[5]);
  chain[11] = block_generator_(chain[6]);
  chain[12] = block_generator_(chain[6]);
  chain[13] = block_generator_(chain[7]);
  chain[14] = block_generator_(chain[7]);
  chain[15] = block_generator_(chain[8]);
  chain[16] = block_generator_(chain[8]);

  // add all the blocks to the chain
  for (std::size_t i = 1; i < chain.size(); ++i)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain[i]));
  }

  // B13 vs. B12
  {
    MainChain::Blocks blocks;
    EXPECT_TRUE(
        chain_->GetPathToCommonAncestor(blocks, chain[13]->body.hash, chain[12]->body.hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {13, 7, 4, 2})));
  }

  // B16 vs. B15
  {
    MainChain::Blocks blocks;
    EXPECT_TRUE(
        chain_->GetPathToCommonAncestor(blocks, chain[16]->body.hash, chain[15]->body.hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {16, 8})));
  }

  // B16 vs. B14
  {
    MainChain::Blocks blocks;
    EXPECT_TRUE(
        chain_->GetPathToCommonAncestor(blocks, chain[16]->body.hash, chain[14]->body.hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {16, 8, 4})));
  }

  // B16 vs. B2
  {
    MainChain::Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, chain[16]->body.hash, chain[2]->body.hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {16, 8, 4, 2})));
  }

  // B1 vs. B16
  {
    MainChain::Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, chain[1]->body.hash, chain[16]->body.hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {1})));
  }

  // B4 vs. B11
  {
    MainChain::Blocks blocks;
    EXPECT_TRUE(chain_->GetPathToCommonAncestor(blocks, chain[4]->body.hash, chain[11]->body.hash));

    // compare against expected results
    ASSERT_TRUE(AreEqual(blocks, Extract(chain, {4, 2})));
  }
}


TEST_F(MainChainSubTreeTests, Check_Common_Ancestor_With_Limit_Exceeded_Yields_Path_Including_Ancestor)
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
  for (auto const &block : Blocks{b1, b2, b3})
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block));
  }

  auto blocks = GetAncestorInLimit(MainChain::BehaviourWhenLimit::RETURN_LEAST_RECENT, b1, b3);

  EXPECT_EQ(b2->body.hash,      blocks[0]->body.hash);
  EXPECT_EQ(genesis->body.hash, blocks[1]->body.hash);
}

TEST_F(MainChainSubTreeTests, Check_Common_Ancestor_With_Limit_Exceeded_Yields_Path_Not_Including_Ancestor)
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
  for (auto const &block : Blocks{b1, b2, b3})
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block));
  }

  auto blocks = GetAncestorInLimit(MainChain::BehaviourWhenLimit::RETURN_MOST_RECENT, b1, b3);

  EXPECT_EQ(b3->body.hash, blocks[0]->body.hash);
  EXPECT_EQ(b2->body.hash, blocks[1]->body.hash);
}

}  // namespace
