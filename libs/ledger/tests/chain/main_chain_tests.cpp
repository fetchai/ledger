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

#include "core/byte_array/byte_array.hpp"
#include "core/containers/set_difference.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"

#include "block_generator.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <random>

using namespace fetch;

using fetch::ledger::Block;
using fetch::ledger::MainChain;
// using fetch::byte_array::ToBase64; // needed for debug messages

using Rng               = std::mt19937_64;
using BlockHash         = Block::Digest;
using MainChainPtr      = std::unique_ptr<MainChain>;
using BlockGeneratorPtr = std::unique_ptr<BlockGenerator>;
using BlockPtr          = BlockGenerator::BlockPtr;

static constexpr char const *LOGGING_NAME = "MainChainTests";

class MainChainTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    static constexpr std::size_t NUM_LANES  = 1;
    static constexpr std::size_t NUM_SLICES = 2;

    chain_     = std::make_unique<MainChain>(true);
    generator_ = std::make_unique<BlockGenerator>(NUM_LANES, NUM_SLICES);
  }

  void TearDown() override
  {
    chain_.reset();
  }

  MainChainPtr      chain_;
  BlockGeneratorPtr generator_;
};

TEST_F(MainChainTests, EnsureGenesisIsConsistent)
{
  auto const genesis = generator_->Generate();
  ASSERT_FALSE(chain_->AddBlock(*genesis));
}

TEST_F(MainChainTests, BuildingOnMainChain)
{
  BlockPtr const genesis = generator_->Generate();

  // Add another 3 blocks in order
  BlockPtr expected_heaviest_block = genesis;
  for (std::size_t i = 0; i < 3; ++i)
  {
    // create a new sequential block
    auto const next_block = generator_->Generate(expected_heaviest_block);

    // add the block to the chain
    ASSERT_TRUE(chain_->AddBlock(*next_block));

    // ensure that this block has become the heaviest
    ASSERT_EQ(chain_->heaviest_block_hash(), next_block->body.hash);
    expected_heaviest_block = next_block;
  }

  // Try adding a non-sequential block (prev hash is itself)
  auto const dummy_block = generator_->Generate(genesis);
  ASSERT_TRUE(chain_->AddBlock(*dummy_block));

  // check that the heaviest block has not changed
  ASSERT_EQ(chain_->heaviest_block_hash(), expected_heaviest_block->body.hash);
}

TEST_F(MainChainTests, CheckSideChainSwitching)
{
  auto const genesis = generator_->Generate();

  // build a small side chain
  auto const side1 = generator_->Generate(genesis);
  auto const side2 = generator_->Generate(side1);

  // build a larger main chain
  auto const main1 = generator_->Generate(genesis);
  auto const main2 = generator_->Generate(main1);
  auto const main3 = generator_->Generate(main2);

  // add the side chain blocks
  ASSERT_TRUE(chain_->AddBlock(*side1));
  ASSERT_EQ(chain_->heaviest_block_hash(), side1->body.hash);
  ASSERT_TRUE(chain_->AddBlock(*side2));
  ASSERT_EQ(chain_->heaviest_block_hash(), side2->body.hash);

  // add the main chain blocks
  ASSERT_TRUE(chain_->AddBlock(*main1));
  ASSERT_EQ(chain_->heaviest_block_hash(), side2->body.hash);
  ASSERT_TRUE(chain_->AddBlock(*main2));
  ASSERT_EQ(chain_->heaviest_block_hash(),
            main2->body.hash);  // this happens to be the case because of the values of the hashes
  ASSERT_TRUE(chain_->AddBlock(*main3));
  ASSERT_EQ(chain_->heaviest_block_hash(), main3->body.hash);
}

#if 1
TEST_F(MainChainTests, CheckChainTipInvalidation)
{
  auto const genesis = generator_->Generate();

  // build a small side chain
  auto const side1 = generator_->Generate(genesis);
  auto const side2 = generator_->Generate(side1);

  // build a larger main chain
  auto const main1 = generator_->Generate(genesis);
  auto const main2 = generator_->Generate(main1);
  auto const main3 = generator_->Generate(main2);

  FETCH_LOG_DEBUG(LOGGING_NAME, "Genesis: ", ToBase64(genesis->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Side 1 : ", ToBase64(side1->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Side 2 : ", ToBase64(side2->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Main 1 : ", ToBase64(main1->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Main 2 : ", ToBase64(main2->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Main 3 : ", ToBase64(main3->body.hash));

  // add the side chain blocks
  ASSERT_TRUE(chain_->AddBlock(*side1));
  ASSERT_EQ(chain_->heaviest_block_hash(), side1->body.hash);
  ASSERT_TRUE(chain_->AddBlock(*side2));
  ASSERT_EQ(chain_->heaviest_block_hash(), side2->body.hash);

  // add the main chain blocks
  ASSERT_TRUE(chain_->AddBlock(*main1));
  ASSERT_EQ(chain_->heaviest_block_hash(), side2->body.hash);
  ASSERT_TRUE(chain_->AddBlock(*main2));
  ASSERT_EQ(chain_->heaviest_block_hash(),
            main2->body.hash);  // this happens to be the case because of the values of the hashes
  ASSERT_TRUE(chain_->AddBlock(*main3));
  ASSERT_EQ(chain_->heaviest_block_hash(), main3->body.hash);

  // invalidate the last entry in the main chain
  ASSERT_TRUE(chain_->InvalidateTip(main3->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Heaviest: ", ToBase64(chain_->heaviest_block_hash()));

  ASSERT_EQ(chain_->heaviest_block_hash(), main2->body.hash);
}
#endif

TEST_F(MainChainTests, CheckChainBlockInvalidation)
{
  auto const genesis = generator_->Generate();

  // build a small side chain
  auto const side1 = generator_->Generate(genesis);
  auto const side2 = generator_->Generate(side1);

  // build a larger main chain
  auto const main1 = generator_->Generate(genesis);
  auto const main2 = generator_->Generate(main1);
  auto const main3 = generator_->Generate(main2);

  FETCH_LOG_DEBUG(LOGGING_NAME, "Genesis : ", ToBase64(genesis->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Side 1  : ", ToBase64(side1->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Side 2  : ", ToBase64(side2->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Main 1  : ", ToBase64(main1->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Main 2  : ", ToBase64(main2->body.hash));
  FETCH_LOG_DEBUG(LOGGING_NAME, "Main 3  : ", ToBase64(main3->body.hash));

  // add the side chain blocks
  ASSERT_TRUE(chain_->AddBlock(*side1));
  ASSERT_EQ(chain_->heaviest_block_hash(), side1->body.hash);
  ASSERT_TRUE(chain_->AddBlock(*side2));
  ASSERT_EQ(chain_->heaviest_block_hash(), side2->body.hash);

  // add the main chain blocks
  ASSERT_TRUE(chain_->AddBlock(*main1));
  ASSERT_EQ(chain_->heaviest_block_hash(), side2->body.hash);
  ASSERT_TRUE(chain_->AddBlock(*main2));
  ASSERT_EQ(chain_->heaviest_block_hash(),
            main2->body.hash);  // this happens to be the case because of the values of the hashes
  ASSERT_TRUE(chain_->AddBlock(*main3));
  ASSERT_EQ(chain_->heaviest_block_hash(), main3->body.hash);

  // invalidate the middle of the side chain this will cause the blocks to be
  ASSERT_TRUE(chain_->InvalidateBlock(main2->body.hash));

  FETCH_LOG_DEBUG(LOGGING_NAME, "Heaviest: ", ToBase64(chain_->heaviest_block_hash()));
  ASSERT_EQ(chain_->heaviest_block_hash(), side2->body.hash);
}

TEST_F(MainChainTests, CheckReindexingOfTips)
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
  std::vector<BlockPtr> chain(17);
  chain[0]  = generator_->Generate();
  chain[1]  = generator_->Generate(chain[0]);
  chain[2]  = generator_->Generate(chain[1]);
  chain[3]  = generator_->Generate(chain[2]);
  chain[4]  = generator_->Generate(chain[2]);
  chain[5]  = generator_->Generate(chain[3]);
  chain[6]  = generator_->Generate(chain[3]);
  chain[7]  = generator_->Generate(chain[4]);
  chain[8]  = generator_->Generate(chain[4]);
  chain[9]  = generator_->Generate(chain[5]);
  chain[10] = generator_->Generate(chain[5]);
  chain[11] = generator_->Generate(chain[6]);
  chain[12] = generator_->Generate(chain[6]);
  chain[13] = generator_->Generate(chain[7]);
  chain[14] = generator_->Generate(chain[7]);
  chain[15] = generator_->Generate(chain[8]);
  chain[16] = generator_->Generate(chain[8]);

  // cache the state of the original tips
  auto const original_tips = chain_->GetTips();

  // force the chain to index its tips
  ASSERT_TRUE(chain_->ReindexTips());

  auto const reindexed_tips = chain_->GetTips();

  // compare the two sets and determine what if any nodes have been added or removed as compared
  // with original
  auto const missing_tips = original_tips - reindexed_tips;
  auto const added_tips   = reindexed_tips - original_tips;

  ASSERT_TRUE(missing_tips.empty());
  ASSERT_TRUE(added_tips.empty());
}
