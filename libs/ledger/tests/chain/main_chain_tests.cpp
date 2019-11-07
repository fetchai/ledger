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

#include "bloom_filter/bloom_filter.hpp"
#include "chain/transaction_layout_rpc_serializers.hpp"
#include "chain/transaction_rpc_serializers.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/containers/set_difference.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/testing/block_generator.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

using namespace fetch;

using fetch::ledger::Block;
using fetch::ledger::MainChain;
using fetch::ledger::BlockStatus;
using fetch::ledger::testing::BlockGenerator;
using fetch::chain::Address;

using Rng               = std::mt19937_64;
using MainChainPtr      = std::unique_ptr<MainChain>;
using BlockGeneratorPtr = std::unique_ptr<BlockGenerator>;
using BlockPtr          = BlockGenerator::BlockPtr;

static bool IsSameBlock(Block const &a, Block const &b)
{
  return (a.body.hash == b.body.hash) && (a.body.previous_hash == b.body.previous_hash) &&
         (a.body.merkle_hash == b.body.merkle_hash) &&
         (a.body.block_number == b.body.block_number) && (a.body.miner == b.body.miner) &&
         (a.body.log2_num_lanes == b.body.log2_num_lanes) &&
         (a.body.timestamp == b.body.timestamp) && (a.body.slices == b.body.slices) &&
         (a.proof == b.proof) && (a.nonce == b.nonce);
}

template <typename Container, typename Value>
static bool Contains(Container const &collection, Value const &value)
{
  return std::find(collection.begin(), collection.end(), value) != collection.end();
}

auto Generate(BlockGeneratorPtr &gen, BlockPtr genesis, std::size_t amount)
{
  std::vector<BlockPtr> retVal(amount);
  for (auto &block : retVal)
  {
    genesis = block = gen->Generate(genesis);
  }
  return retVal;
}

std::ostream &Print(std::ostream &s, fetch::Digest const &hash)
{
  s << '#' << std::hex;
  if (hash.empty())
  {
    return s << "<nil>";
  }
  for (std::size_t i{}; i < 4 && i < hash.size(); ++i)
  {
    s << int(hash[i]);
  }
  return s;
}

std::string Hashes(std::vector<BlockPtr> const &v)
{
  if (v.empty())
  {
    return "<nil>";
  }
  std::ostringstream ret_val;
  auto               block{v.begin()};
  Print(ret_val, (*block)->body.hash);
  for (++block; block != v.end(); ++block)
  {
    Print(ret_val << ", ", (*block)->body.hash);
  }
  return ret_val.str();
}

class MainChainTests : public ::testing::TestWithParam<MainChain::Mode>
{
protected:
  void SetUp() override
  {
    fetch::crypto::mcl::details::MCLInitialiser();

    static constexpr std::size_t NUM_LANES  = 1;
    static constexpr std::size_t NUM_SLICES = 2;

    auto const main_chain_mode = GetParam();

    chain_     = std::make_unique<MainChain>(false, main_chain_mode);
    generator_ = std::make_unique<BlockGenerator>(NUM_LANES, NUM_SLICES);
  }

  MainChainPtr      chain_;
  BlockGeneratorPtr generator_;
};

TEST_P(MainChainTests, EnsureGenesisIsConsistent)
{
  auto const genesis = generator_->Generate();
  ASSERT_EQ(BlockStatus::DUPLICATE, chain_->AddBlock(*genesis));
}

TEST_P(MainChainTests, BuildingOnMainChain)
{
  BlockPtr const genesis = generator_->Generate();

  // Add another 3 blocks in order
  BlockPtr expected_heaviest_block = genesis;
  for (std::size_t i = 0; i < 3; ++i)
  {
    // create a new sequential block
    auto const next_block = generator_->Generate(expected_heaviest_block);

    // add the block to the chain
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*next_block));

    // ensure that this block has become the heaviest
    ASSERT_EQ(chain_->GetHeaviestBlockHash(), next_block->body.hash);
    expected_heaviest_block = next_block;
  }

  // Try adding a non-sequential block (prev hash is itself)
  auto const dummy_block = generator_->Generate(genesis);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*dummy_block));

  // check that the heaviest block has not changed
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), expected_heaviest_block->body.hash);
}

TEST_P(MainChainTests, CheckSideChainSwitching)
{
  auto const genesis = generator_->Generate();

  // build a small side chain
  auto const side{Generate(generator_, genesis, 2)};

  // build a larger main chain
  auto const main{Generate(generator_, genesis, 3)};

  // add the side chain blocks
  for (auto const &block : side)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block)) << "; side: " << Hashes(side);
    ASSERT_EQ(chain_->GetHeaviestBlockHash(), block->body.hash)
        << "when adding side block no. " << block->body.block_number << "; side: " << Hashes(side);
  }

  // add the main chain blocks
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main.front()))
      << "; side: " << Hashes(side) << "; main: " << Hashes(main);
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), side.back()->body.hash)
      << "; side: " << Hashes(side) << "; main: " << Hashes(main);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main[1]))
      << "; side: " << Hashes(side) << "; main: " << Hashes(main);
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), std::max(main[1]->body.hash, side.back()->body.hash))
      << "; side: " << Hashes(side) << "; main: " << Hashes(main);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main.back()))
      << "; side: " << Hashes(side) << "; main: " << Hashes(main);
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main.back()->body.hash)
      << "; side: " << Hashes(side) << "; main: " << Hashes(main);
}

TEST_P(MainChainTests, CheckChainBlockInvalidation)
{
  auto const genesis = generator_->Generate();

  // build a few branches
  auto const branch3{Generate(generator_, genesis, 3)};
  auto const branch5{Generate(generator_, genesis, 5)};
  auto const branch9{Generate(generator_, genesis, 9)};

  // an offspring of branch9
  std::vector<BlockGenerator::BlockPtr> branch7(branch9.cbegin(), branch9.cbegin() + 3);
  for (auto &&other_direction : Generate(generator_, branch7.back(), 4))
  {
    branch7.emplace_back(std::move(other_direction));
  }

  auto const branch6{Generate(generator_, genesis, 6)};

#ifdef FETCH_LOG_DEBUG_ENABLED
  FETCH_LOG_DEBUG(LOGGING_NAME, "Genesis : ", fetch::byte_array::ToBase64(genesis->body.hash));
  for (auto const &branch : {branch3, branch5, branch9, branch7, branch6})
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Branch", branch.size(), ": ", Hashes(branch));
  }
#endif

  // add the initial branch 3
  for (auto const &block : branch3)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block)) << "branch3: " << Hashes(branch3);
    ASSERT_EQ(chain_->GetHeaviestBlockHash(), block->body.hash)
        << "when adding branch3's block no. " << block->body.hash
        << "; branch3: " << Hashes(branch3);
  }

  // and the two more branches growing in length
  auto youngest_block_age{branch3.size() - 1};
  auto best_block{branch3.back()};

  for (auto const &branch : {branch5, branch9})
  {
    for (std::size_t i{}; i < youngest_block_age; ++i)
    {
      ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*branch[i]))
          << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
          << "; branch9: " << Hashes(branch9);
      ASSERT_EQ(chain_->GetHeaviestBlockHash(), best_block->body.hash)
          << "when adding branch" << branch.size() << "'s block no. " << i
          << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
          << "; branch9: " << Hashes(branch9);
    }
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*branch[youngest_block_age]))
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch9: " << Hashes(branch9);
    ASSERT_EQ(chain_->GetHeaviestBlockHash(),
              std::max(best_block->body.hash, branch[youngest_block_age]->body.hash));
    for (std::size_t i{youngest_block_age + 1}; i < branch.size(); ++i)
    {
      ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*branch[i]))
          << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
          << "; branch9: " << Hashes(branch9);
      ASSERT_EQ(chain_->GetHeaviestBlockHash(), branch[i]->body.hash)
          << "when adding branch" << branch.size() << "'s block no. " << i
          << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
          << "; branch9: " << Hashes(branch9);
    }
    youngest_block_age = branch.size() - 1;
    best_block         = branch.back();
  }

  // now branch9 is the best, the two remaining won't change the heaviest block
  for (std::size_t i{}; i < 3; ++i)
  {
    ASSERT_EQ(BlockStatus::DUPLICATE, chain_->AddBlock(*branch7[i]))
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch7: " << Hashes(branch7) << "; branch9: " << Hashes(branch9);
  }
  for (std::size_t i{3}; i < 7; ++i)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*branch7[i]))
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch7: " << Hashes(branch7) << "; branch9: " << Hashes(branch9);
    ASSERT_EQ(chain_->GetHeaviestBlockHash(), best_block->body.hash)
        << "when adding branch7's block no. " << branch7[i]->body.block_number
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch7: " << Hashes(branch7) << "; branch9: " << Hashes(branch9);
  }
  for (auto const &block : branch6)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block))
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
    ASSERT_EQ(chain_->GetHeaviestBlockHash(), best_block->body.hash)
        << "when adding branch6's block no. " << block->body.block_number
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
  }

  // invalidate the middle of the longest branch, now branch7 is the best
  ASSERT_TRUE(chain_->RemoveBlock(branch9[6]->body.hash))
      << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
      << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
      << "; branch9: " << Hashes(branch9);
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), branch7.back()->body.hash)
      << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
      << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
      << "; branch9: " << Hashes(branch9);

  // check that three blocks have been removed
  for (std::size_t i{}; i < 6; ++i)
  {
    ASSERT_TRUE(static_cast<bool>(chain_->GetBlock(branch9[i]->body.hash)))
        << "when searching block no. " << i << " of branch9"
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
  }
  for (std::size_t i{6}; i < branch9.size(); ++i)
  {
    ASSERT_FALSE(static_cast<bool>(chain_->GetBlock(branch9[i]->body.hash)))
        << "when searching block no. " << i << " of branch9"
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
  }

  // go on cutting branches
  ASSERT_TRUE(chain_->RemoveBlock(branch7[2]->body.hash))
      << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
      << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
      << "; branch9: " << Hashes(branch9);
  // now both branch7 and branch9 have almost been wiped out of the chain and branch6 is the
  // heaviest
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), branch6.back()->body.hash)
      << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
      << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
      << "; branch9: " << Hashes(branch9);
  for (std::size_t i{}; i < 2; ++i)
  {
    ASSERT_TRUE(static_cast<bool>(chain_->GetBlock(branch9[i]->body.hash)))
        << "when searching block no. " << i << " of branch9"
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
  }
  for (std::size_t i{2}; i < branch9.size(); ++i)
  {
    ASSERT_FALSE(static_cast<bool>(chain_->GetBlock(branch9[i]->body.hash)))
        << "when searching block no. " << i << " of branch9"
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
  }
  for (std::size_t i{2}; i < branch7.size(); ++i)
  {
    ASSERT_FALSE(static_cast<bool>(chain_->GetBlock(branch7[i]->body.hash)))
        << "when searching block no. " << i << " of branch7"
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
  }
  ASSERT_TRUE(chain_->RemoveBlock(branch6[4]->body.hash));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), branch5.back()->body.hash);
  for (std::size_t i{}; i < 4; ++i)
  {
    ASSERT_TRUE(static_cast<bool>(chain_->GetBlock(branch6[i]->body.hash)))
        << "when searching block no. " << i << " of branch9"
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
  }
  for (std::size_t i{4}; i < branch6.size(); ++i)
  {
    ASSERT_FALSE(static_cast<bool>(chain_->GetBlock(branch6[i]->body.hash)))
        << "when searching block no. " << i << " of branch9"
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
  }
}

TEST_P(MainChainTests, CheckReindexingOfTips)
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

  // add all the tips
  for (std::size_t i = 1; i < chain.size(); ++i)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain[i]));
  }

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

TEST_P(MainChainTests, CheckReindexingOfWithLooseTips)
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

  // add all the tips
  for (std::size_t i = 1; i < chain.size(); ++i)
  {
    // introduce a loose set of tips
    if (i == 7)
    {
      continue;
    }
    if (i == 13 || i == 14)
    {
      ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*chain[i]));
    }
    else
    {
      ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain[i]));
    }
  }

  // cache the state of the original tips
  auto const original_tips  = chain_->GetTips();
  auto const missing_hashes = chain_->GetMissingBlockHashes();
  ASSERT_TRUE(!missing_hashes.empty());

  // force the chain to index its tips
  ASSERT_TRUE(chain_->ReindexTips());

  auto const reindexed_tips = chain_->GetTips();

  // compare the two sets and determine what if any nodes have been added or removed as compared
  // with original
  auto const missing_tips = original_tips - reindexed_tips;
  auto const added_tips   = reindexed_tips - original_tips;

  ASSERT_TRUE(missing_tips.empty());
  ASSERT_TRUE(added_tips.empty());

  // check the missing hashes
  auto const after_missing_hashes = chain_->GetMissingBlockHashes();
  ASSERT_EQ(after_missing_hashes, missing_hashes);
}

TEST_P(MainChainTests, BuilingOnMainChain)
{
  auto genesis = generator_->Generate();

  auto main1 = generator_->Generate(genesis);
  auto main2 = generator_->Generate(main1);
  auto main3 = generator_->Generate(main2);

  // ensure the genesis block is valid
  EXPECT_EQ(genesis->body.block_number, 0);

  // add all the blocks
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main1));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main1->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main2->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main3));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main3->body.hash);

  // side chain
  auto side1 = generator_->Generate(genesis);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*side1));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main3->body.hash);
}

TEST_P(MainChainTests, AdditionOfBlocksOutOfOrder)
{
  auto genesis = generator_->Generate();
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), genesis->body.hash);

  // Try adding a non-sequential block (prev hash is itself)
  Block dummy;
  dummy.body.block_number = 2;
  dummy.body.miner        = chain::Address{chain::Address::RawAddress{}};
  dummy.UpdateDigest();
  dummy.body.previous_hash = dummy.body.hash;

  ASSERT_EQ(BlockStatus::INVALID, chain_->AddBlock(dummy));
  EXPECT_EQ(chain_->GetHeaviestBlockHash(), genesis->body.hash);

  // build a main chain
  auto main1 = generator_->Generate(genesis);
  auto main2 = generator_->Generate(main1);
  auto main3 = generator_->Generate(main2);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main1));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main1->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main2->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main3));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main3->body.hash);
}

TEST_P(MainChainTests, AdditionOfBlocksWithABreak)
{
  auto genesis = generator_->Generate();
  auto main1   = generator_->Generate(genesis);
  auto main2   = generator_->Generate(main1);
  auto main3   = generator_->Generate(main2);
  auto main4   = generator_->Generate(main3);
  auto main5   = generator_->Generate(main4);  // break
  auto main6   = generator_->Generate(main5);
  auto main7   = generator_->Generate(main6);
  auto main8   = generator_->Generate(main7);
  auto main9   = generator_->Generate(main8);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main1));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main1->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main2->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main3));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main3->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main4));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main4->body.hash);

  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*main6));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main4->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*main7));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main4->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*main8));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main4->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*main9));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main4->body.hash);
}

TEST_P(MainChainTests, CheckChainPreceeding)
{
  auto genesis = generator_->Generate();
  auto main1   = generator_->Generate(genesis);
  auto main2   = generator_->Generate(main1);
  auto main3   = generator_->Generate(main2);
  auto main4   = generator_->Generate(main3);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main1));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main1->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main2->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main3));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main3->body.hash);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main4));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main4->body.hash);

  {
    auto const preceding = chain_->GetChainPreceding(main4->body.hash, 2);
    ASSERT_EQ(preceding.size(), 2);
    EXPECT_TRUE(IsSameBlock(*preceding[0], *main4));
    EXPECT_TRUE(IsSameBlock(*preceding[1], *main3));
  }

  {
    auto const preceding = chain_->GetChainPreceding(main3->body.hash, 2);
    ASSERT_EQ(preceding.size(), 2);
    EXPECT_TRUE(IsSameBlock(*preceding[0], *main3));
    EXPECT_TRUE(IsSameBlock(*preceding[1], *main2));
  }

  {
    auto const preceding = chain_->GetChainPreceding(main2->body.hash, 2);
    ASSERT_EQ(preceding.size(), 2);
    EXPECT_TRUE(IsSameBlock(*preceding[0], *main2));
    EXPECT_TRUE(IsSameBlock(*preceding[1], *main1));
  }

  {
    auto const preceding = chain_->GetChainPreceding(main1->body.hash, 3);
    ASSERT_EQ(preceding.size(), 2);
    EXPECT_TRUE(IsSameBlock(*preceding[0], *main1));
    EXPECT_TRUE(IsSameBlock(*preceding[1], *genesis));
  }

  {
    auto const heaviest = chain_->GetHeaviestChain(2);
    ASSERT_EQ(heaviest.size(), 2);
    EXPECT_TRUE(IsSameBlock(*heaviest[0], *main4));
    EXPECT_TRUE(IsSameBlock(*heaviest[1], *main3));
  }
}

TEST_P(MainChainTests, CheckMissingLooseBlocks)
{
  auto genesis = generator_->Generate();
  auto main1   = generator_->Generate(genesis);
  auto main2   = generator_->Generate(main1);
  auto main3   = generator_->Generate(main2);
  auto main4   = generator_->Generate(main3);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main1));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main1->body.hash);

  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*main3));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main1->body.hash);

  ASSERT_TRUE(chain_->HasMissingBlocks());
  auto const missing_hashes = chain_->GetMissingBlockHashes(MainChain::UPPER_BOUND);

  ASSERT_EQ(missing_hashes.size(), 1);
  ASSERT_EQ(missing_hashes[0], main2->body.hash);

  auto const tips = chain_->GetTips();
  ASSERT_EQ(tips.size(), 1);
  ASSERT_EQ(*tips.cbegin(), main1->body.hash);
}

TEST_P(MainChainTests, CheckMultipleMissing)
{
  auto genesis = generator_->Generate();
  auto common1 = generator_->Generate(genesis);

  auto side1_1 = generator_->Generate(common1);
  auto side1_2 = generator_->Generate(side1_1);

  auto side2_1 = generator_->Generate(common1);
  auto side2_2 = generator_->Generate(side2_1);

  auto side3_1 = generator_->Generate(common1);
  auto side3_2 = generator_->Generate(side3_1);

  auto side4_1 = generator_->Generate(common1);
  auto side4_2 = generator_->Generate(side4_1);

  auto side5_1 = generator_->Generate(common1);
  auto side5_2 = generator_->Generate(side5_1);

  // add the common block
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*common1));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);

  // add all the side chain tips
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side1_2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side2_2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side3_2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side4_2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side5_2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);

  // check the missing hashes
  auto const all_missing = chain_->GetMissingBlockHashes();

  ASSERT_EQ(all_missing.size(), 5);
  ASSERT_TRUE(Contains(all_missing, side1_1->body.hash));
  ASSERT_TRUE(Contains(all_missing, side2_1->body.hash));
  ASSERT_TRUE(Contains(all_missing, side3_1->body.hash));
  ASSERT_TRUE(Contains(all_missing, side4_1->body.hash));
  ASSERT_TRUE(Contains(all_missing, side5_1->body.hash));

  // ensure that this is a subset of the missing blocks
  auto const subset_missing = chain_->GetMissingBlockHashes(3);
  for (auto const &missing : subset_missing)
  {
    ASSERT_TRUE(Contains(all_missing, missing));
  }
}

TEST_P(MainChainTests, CheckLongChainWrite)
{
  static constexpr std::size_t NUM_BLOCKS = 30;

  auto previous_block = generator_->Generate();

  std::vector<BlockPtr> blocks{NUM_BLOCKS};
  for (std::size_t i = 0; i < NUM_BLOCKS; ++i)
  {
    // generate the next block in the sequence
    auto next_block = generator_->Generate(previous_block);

    // add it to the chain
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*next_block));

    // update the previous block
    previous_block = next_block;

    // cache all the blocks
    blocks[i] = next_block;
  }

  for (auto const &block : blocks)
  {
    auto const retrived_block = chain_->GetBlock(block->body.hash);
    ASSERT_TRUE(retrived_block);
    ASSERT_EQ(retrived_block->body.hash, block->body.hash);
  }
}

TEST_P(MainChainTests, CheckInOrderWeights)
{
  auto genesis = generator_->Generate();
  auto main1   = generator_->Generate(genesis);
  auto main2   = generator_->Generate(main1);
  auto main3   = generator_->Generate(main2);
  auto main4   = generator_->Generate(main3);
  auto main5   = generator_->Generate(main4);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main1));
  ASSERT_FALSE(chain_->HasMissingBlocks());

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main2));
  ASSERT_FALSE(chain_->HasMissingBlocks());

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main3));
  ASSERT_FALSE(chain_->HasMissingBlocks());

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main4));
  ASSERT_FALSE(chain_->HasMissingBlocks());

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main5));
  ASSERT_FALSE(chain_->HasMissingBlocks());

  // check the weights
  ASSERT_EQ(chain_->GetBlock(main1->body.hash)->total_weight, main1->total_weight);
  ASSERT_EQ(chain_->GetBlock(main2->body.hash)->total_weight, main2->total_weight);
  ASSERT_EQ(chain_->GetBlock(main3->body.hash)->total_weight, main3->total_weight);
  ASSERT_EQ(chain_->GetBlock(main4->body.hash)->total_weight, main4->total_weight);
  ASSERT_EQ(chain_->GetBlock(main5->body.hash)->total_weight, main5->total_weight);
}

TEST_P(MainChainTests, CheckResolvedLooseWeight)
{
  auto genesis = generator_->Generate();
  auto other   = generator_->Generate(genesis);
  auto main1   = generator_->Generate(other);
  auto main2   = generator_->Generate(main1);
  auto main3   = generator_->Generate(main2);
  auto main4   = generator_->Generate(main3);
  auto main5   = generator_->Generate(main4);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*other));
  ASSERT_FALSE(chain_->HasMissingBlocks());
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), other->body.hash);

  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*main5));
  ASSERT_TRUE(chain_->HasMissingBlocks());
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), other->body.hash);

  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*main4));
  ASSERT_TRUE(chain_->HasMissingBlocks());
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), other->body.hash);

  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*main3));
  ASSERT_TRUE(chain_->HasMissingBlocks());
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), other->body.hash);

  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*main2));
  ASSERT_TRUE(chain_->HasMissingBlocks());
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), other->body.hash);

  // this block resolves all the loose blocks
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main1));
  ASSERT_FALSE(chain_->HasMissingBlocks());
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), main5->body.hash);

  ASSERT_EQ(chain_->GetBlock(main1->body.hash)->total_weight, main1->total_weight);
  ASSERT_EQ(chain_->GetBlock(main2->body.hash)->total_weight, main2->total_weight);
  ASSERT_EQ(chain_->GetBlock(main3->body.hash)->total_weight, main3->total_weight);
  ASSERT_EQ(chain_->GetBlock(main4->body.hash)->total_weight, main4->total_weight);
  ASSERT_EQ(chain_->GetBlock(main5->body.hash)->total_weight, main5->total_weight);
}

INSTANTIATE_TEST_CASE_P(ParamBased, MainChainTests,
                        ::testing::Values(MainChain::Mode::CREATE_PERSISTENT_DB,
                                          MainChain::Mode::IN_MEMORY_DB), );
