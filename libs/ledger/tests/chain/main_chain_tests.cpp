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
#include "core/containers/set_intersection.hpp"
#include "core/random/lcg.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/consensus/consensus.hpp"
#include "ledger/testing/block_generator.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <random>
#include <sstream>
#include <vector>

using namespace fetch;

using fetch::ledger::Block;
using fetch::ledger::MainChain;
using fetch::ledger::BlockStatus;
using fetch::ledger::testing::BlockGenerator;
using fetch::ledger::Consensus;
using fetch::chain::Address;

using Rng               = std::mt19937_64;
using MainChainPtr      = std::unique_ptr<MainChain>;
using BlockGeneratorPtr = std::unique_ptr<BlockGenerator>;
using BlockPtr          = BlockGenerator::BlockPtr;
using BlockHash         = MainChain::BlockHash;

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

auto Generate(BlockGeneratorPtr &gen, Block::BlockEntropy::Cabinet const &cabinet,
              BlockPtr const &previous, uint64_t weight = 0)
{
  BlockPtr block;

  block = gen->Generate(previous);
  // If first block from genesis then fill with cabinet members
  if (previous->body.block_number == 0)
  {
    assert(!cabinet.empty());
    block->body.block_entropy.qualified = cabinet;
    // Insert fake confirmatin to trigger aeon beginning
    block->body.block_entropy.confirmations.insert({"fake", "confirmation"});
  }

  // If no weight specified pick random miner to make block
  if (weight == 0)
  {
    block->body.miner_id = crypto::Identity{*std::next(cabinet.begin(), rand() % cabinet.size())};
    block->weight        = Consensus::ShuffledCabinetRank(cabinet, *block, block->body.miner_id);
    block->total_weight  = previous->total_weight + block->weight;
    return block;
  }

  assert(weight <= cabinet.size());
  block->weight       = weight;
  block->total_weight = previous->total_weight + block->weight;
  for (auto const &member : cabinet)
  {
    auto member_id = crypto::Identity{member};
    if (weight == Consensus::ShuffledCabinetRank(cabinet, *block, member_id))
    {
      block->body.miner_id = member_id;
      break;
    }
  }
  return block;
}

auto GenerateChain(BlockGeneratorPtr &gen, Block::BlockEntropy::Cabinet const &cabinet,
                   BlockPtr const &start, uint64_t length,
                   std::vector<std::vector<BlockPtr>> const &side_chain_list)
{
  std::vector<BlockPtr> retVal(length);

  // Initialise to the chain position to the first element of all side chains
  std::vector<uint8_t> chain_position(side_chain_list.size(), 0);

  // Generate blocks
  assert(!cabinet.empty());
  auto previous = start;
  for (auto &block : retVal)
  {
    // Generate weights which need to be coordinated with side chains if there is one
    std::unordered_set<uint64_t> side_miners{};
    for (auto i = 0; i < side_chain_list.size(); ++i)
    {
      if (chain_position[i] < side_chain_list[i].size())
      {
        side_miners.insert(side_chain_list[i][chain_position[i]]->weight);
        ++chain_position[i];
      }
    }

    // Choose a random weight not in side miners
    std::unordered_set<uint64_t> all_weights;
    for (uint64_t i = 1; i <= cabinet.size(); ++i)
    {
      all_weights.insert(all_weights.end(), i);
    }
    auto diff = all_weights - side_miners;

    block    = Generate(gen, cabinet, previous,
                     *std::next(diff.begin(), static_cast<uint64_t>(rand() % diff.size())));
    previous = block;
  }

  return retVal;
}

auto GenerateChain(BlockGeneratorPtr &gen, Block::BlockEntropy::Cabinet const &cabinet,
                   BlockPtr const &start, uint64_t length = 1, uint64_t block_weight = 0)
{
  std::vector<BlockPtr> retVal(length);

  // Generate blocks
  assert(!cabinet.empty());
  auto previous = start;
  for (auto &block : retVal)
  {
    block    = Generate(gen, cabinet, previous, block_weight);
    previous = block;
  }

  return retVal;
}

Block::Hash GetHeaviestHash(BlockPtr const &block1, BlockPtr const &block2)
{
  Block::Hash heaviest_hash = block1->body.hash;
  if (block2->total_weight > block1->total_weight ||
      (block2->total_weight == block1->total_weight &&
       block2->body.block_number > block1->body.block_number) ||
      (block2->total_weight == block1->total_weight &&
       block2->body.block_number == block1->body.block_number && block2->weight > block1->weight))
  {
    heaviest_hash = block2->body.hash;
  }
  return heaviest_hash;
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

    static constexpr std::size_t NUM_LANES    = 1;
    static constexpr std::size_t NUM_SLICES   = 2;
    static constexpr std::size_t CABINET_SIZE = 8;

    auto const main_chain_mode = GetParam();

    chain_     = std::make_unique<MainChain>(false, main_chain_mode, true);
    generator_ = std::make_unique<BlockGenerator>(NUM_LANES, NUM_SLICES);
    for (auto i = 0; i < CABINET_SIZE; ++i)
    {
      cabinet_.insert("Miner " + std::to_string(i));
    }
  }

  MainChainPtr                 chain_;
  BlockGeneratorPtr            generator_;
  Block::BlockEntropy::Cabinet cabinet_;
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
    auto const next_block = Generate(generator_, cabinet_, expected_heaviest_block);

    // add the block to the chain
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*next_block));

    // ensure that this block has become the heaviest
    ASSERT_EQ(chain_->GetHeaviestBlockHash(), next_block->body.hash);
    expected_heaviest_block = next_block;
  }

  // Try adding a non-sequential block (prev hash is itself)
  auto const dummy_block = Generate(generator_, cabinet_, genesis);
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*dummy_block));

  // check that the heaviest block has not changed
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), expected_heaviest_block->body.hash);
}

TEST_P(MainChainTests, CheckSideChainSwitching)
{
  auto const genesis = generator_->Generate();

  // build a small side chain
  auto const side{GenerateChain(generator_, cabinet_, genesis, 9)};
  // add the side chain blocks
  for (auto const &block : side)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*block)) << "; side: " << Hashes(side);
    ASSERT_EQ(chain_->GetHeaviestBlockHash(), block->body.hash)
        << "when adding side block no. " << block->body.block_number << "; side: " << Hashes(side);
  }

  // build a heavier main chain which must always be heavier than the side chain
  auto const main{GenerateChain(generator_, cabinet_, genesis, 72, {side})};

  // add the main chain blocks
  for (auto i = 0; i < main.size(); ++i)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*main[i]))
        << "; side: " << Hashes(side) << "; main: " << Hashes(main);
    if (i == 0)
    {
      ASSERT_EQ(chain_->GetHeaviestBlockHash(), side.back()->body.hash)
          << "; side: " << Hashes(side) << "; main: " << Hashes(main);
    }
    else if (i == main.size() - 1)
    {
      ASSERT_EQ(chain_->GetHeaviestBlockHash(), main.back()->body.hash)
          << "; side: " << Hashes(side) << "; main: " << Hashes(main);
    }
    else
    {
      ASSERT_EQ(chain_->GetHeaviestBlockHash(), GetHeaviestHash(side.back(), main[i]))
          << "; side: " << Hashes(side) << "; main: " << Hashes(main);
    }
  }
}

TEST_P(MainChainTests, CheckChainBlockInvalidation)
{
  auto const genesis = generator_->Generate();

  // build a few branches
  auto const branch3{GenerateChain(generator_, cabinet_, genesis, 3, 1)};
  auto const branch5{GenerateChain(generator_, cabinet_, genesis, 5, 2)};
  auto const branch9{GenerateChain(generator_, cabinet_, genesis, 9, 5)};

  // an offspring of branch9
  std::vector<BlockGenerator::BlockPtr> branch7(branch9.cbegin(), branch9.cbegin() + 3);
  for (auto &&other_direction : GenerateChain(generator_, cabinet_, branch7.back(), 4, 4))
  {
    branch7.emplace_back(std::move(other_direction));
  }

  auto const branch6{GenerateChain(generator_, cabinet_, genesis, 6, 3)};

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

  // and the two more branches growing in weight
  auto youngest_block_age{branch3.size() - 1};
  auto best_block{branch3.back()};

  for (auto const &branch : {branch5, branch9})
  {
    for (std::size_t i{}; i < youngest_block_age; ++i)
    {
      ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*branch[i]))
          << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
          << "; branch9: " << Hashes(branch9);
      ASSERT_EQ(chain_->GetHeaviestBlockHash(), GetHeaviestHash(best_block, branch[i]))
          << "when adding branch" << branch.size() << "'s block no. " << i
          << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
          << "; branch9: " << Hashes(branch9);
    }
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*branch[youngest_block_age]))
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch9: " << Hashes(branch9);
    ASSERT_EQ(chain_->GetHeaviestBlockHash(),
              GetHeaviestHash(best_block, branch[youngest_block_age]));
    for (std::size_t i{youngest_block_age + 1}; i < branch.size(); ++i)
    {
      ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*branch[i]))
          << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
          << "; branch9: " << Hashes(branch9);
      ASSERT_EQ(chain_->GetHeaviestBlockHash(), GetHeaviestHash(branch[i], best_block))
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
  for (std::size_t i{3}; i < branch7.size(); ++i)
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
  assert(branch9.size() > 6);
  ASSERT_TRUE(chain_->RemoveBlock(branch9[6]->body.hash))
      << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
      << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
      << "; branch9: " << Hashes(branch9);
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), GetHeaviestHash(branch7.back(), branch9[5]))
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
  assert(branch7.size() > 2);
  ASSERT_TRUE(chain_->RemoveBlock(branch7[2]->body.hash))
      << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
      << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
      << "; branch9: " << Hashes(branch9);
  // now both branch7 and branch9 have almost been wiped out of the chain and branch6 is the
  // heaviest
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), GetHeaviestHash(branch6.back(), branch7[1]))
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
  assert(branch6.size() > 3);
  ASSERT_TRUE(chain_->RemoveBlock(branch6[3]->body.hash));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), GetHeaviestHash(branch5.back(), branch6[2]));
  for (std::size_t i{}; i < 3; ++i)
  {
    ASSERT_TRUE(static_cast<bool>(chain_->GetBlock(branch6[i]->body.hash)))
        << "when searching block no. " << i << " of branch9"
        << "; branch3: " << Hashes(branch3) << "; branch5: " << Hashes(branch5)
        << "; branch6: " << Hashes(branch6) << "; branch7: " << Hashes(branch7)
        << "; branch9: " << Hashes(branch9);
  }
  for (std::size_t i{3}; i < branch6.size(); ++i)
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
  // Need to give blocks with the same block number different weights
  chain[0]  = generator_->Generate();
  chain[1]  = Generate(generator_, cabinet_, chain[0]);
  chain[2]  = Generate(generator_, cabinet_, chain[1]);
  chain[3]  = Generate(generator_, cabinet_, chain[2], 3);
  chain[4]  = Generate(generator_, cabinet_, chain[2], 4);
  chain[5]  = Generate(generator_, cabinet_, chain[3], 3);
  chain[6]  = Generate(generator_, cabinet_, chain[3], 4);
  chain[7]  = Generate(generator_, cabinet_, chain[4], 5);
  chain[8]  = Generate(generator_, cabinet_, chain[4], 6);
  chain[9]  = Generate(generator_, cabinet_, chain[5], 1);
  chain[10] = Generate(generator_, cabinet_, chain[5], 2);
  chain[11] = Generate(generator_, cabinet_, chain[6], 3);
  chain[12] = Generate(generator_, cabinet_, chain[6], 4);
  chain[13] = Generate(generator_, cabinet_, chain[7], 5);
  chain[14] = Generate(generator_, cabinet_, chain[7], 6);
  chain[15] = Generate(generator_, cabinet_, chain[8], 7);
  chain[16] = Generate(generator_, cabinet_, chain[8], 8);

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
  // Need to give blocks with the same block number different weights
  chain[0]  = generator_->Generate();
  chain[1]  = Generate(generator_, cabinet_, chain[0]);
  chain[2]  = Generate(generator_, cabinet_, chain[1]);
  chain[3]  = Generate(generator_, cabinet_, chain[2], 3);
  chain[4]  = Generate(generator_, cabinet_, chain[2], 4);
  chain[5]  = Generate(generator_, cabinet_, chain[3], 3);
  chain[6]  = Generate(generator_, cabinet_, chain[3], 4);
  chain[7]  = Generate(generator_, cabinet_, chain[4], 5);
  chain[8]  = Generate(generator_, cabinet_, chain[4], 6);
  chain[9]  = Generate(generator_, cabinet_, chain[5], 1);
  chain[10] = Generate(generator_, cabinet_, chain[5], 2);
  chain[11] = Generate(generator_, cabinet_, chain[6], 3);
  chain[12] = Generate(generator_, cabinet_, chain[6], 4);
  chain[13] = Generate(generator_, cabinet_, chain[7], 5);
  chain[14] = Generate(generator_, cabinet_, chain[7], 6);
  chain[15] = Generate(generator_, cabinet_, chain[8], 7);
  chain[16] = Generate(generator_, cabinet_, chain[8], 8);

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

  auto main1 = Generate(generator_, cabinet_, genesis, 1);
  auto main2 = Generate(generator_, cabinet_, main1);
  auto main3 = Generate(generator_, cabinet_, main2);

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
  auto side1 = Generate(generator_, cabinet_, genesis, 2);

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
  auto main1 = Generate(generator_, cabinet_, genesis);
  auto main2 = Generate(generator_, cabinet_, main1);
  auto main3 = Generate(generator_, cabinet_, main2);

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
  auto main1   = Generate(generator_, cabinet_, genesis);
  auto main2   = Generate(generator_, cabinet_, main1);
  auto main3   = Generate(generator_, cabinet_, main2);
  auto main4   = Generate(generator_, cabinet_, main3);
  auto main5   = Generate(generator_, cabinet_, main4);  // break
  auto main6   = Generate(generator_, cabinet_, main5);
  auto main7   = Generate(generator_, cabinet_, main6);
  auto main8   = Generate(generator_, cabinet_, main7);
  auto main9   = Generate(generator_, cabinet_, main8);

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
  auto main1   = Generate(generator_, cabinet_, genesis);
  auto main2   = Generate(generator_, cabinet_, main1);
  auto main3   = Generate(generator_, cabinet_, main2);
  auto main4   = Generate(generator_, cabinet_, main3);

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
  auto main1   = Generate(generator_, cabinet_, genesis);
  auto main2   = Generate(generator_, cabinet_, main1);
  auto main3   = Generate(generator_, cabinet_, main2);
  auto main4   = Generate(generator_, cabinet_, main3);

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
  auto common1 = Generate(generator_, cabinet_, genesis);

  auto side1 = GenerateChain(generator_, cabinet_, common1, 2);
  auto side2 = GenerateChain(generator_, cabinet_, common1, 2, {side1});
  auto side3 = GenerateChain(generator_, cabinet_, common1, 2, {side1, side2});
  auto side4 = GenerateChain(generator_, cabinet_, common1, 2, {side1, side2, side3});
  auto side5 = GenerateChain(generator_, cabinet_, common1, 2, {side1, side2, side3, side4});

  // add the common block
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*common1));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);

  // add all the second blocks in the side chains
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side1[1]));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side2[1]));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side3[1]));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side4[1]));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);
  ASSERT_EQ(BlockStatus::LOOSE, chain_->AddBlock(*side5[1]));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), common1->body.hash);

  // check the missing hashes
  auto const all_missing = chain_->GetMissingBlockHashes();

  ASSERT_EQ(all_missing.size(), 5);
  ASSERT_TRUE(Contains(all_missing, side1[0]->body.hash));
  ASSERT_TRUE(Contains(all_missing, side2[0]->body.hash));
  ASSERT_TRUE(Contains(all_missing, side3[0]->body.hash));
  ASSERT_TRUE(Contains(all_missing, side4[0]->body.hash));
  ASSERT_TRUE(Contains(all_missing, side5[0]->body.hash));

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
    auto next_block = Generate(generator_, cabinet_, previous_block);

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
  auto main1   = Generate(generator_, cabinet_, genesis);
  auto main2   = Generate(generator_, cabinet_, main1);
  auto main3   = Generate(generator_, cabinet_, main2);
  auto main4   = Generate(generator_, cabinet_, main3);
  auto main5   = Generate(generator_, cabinet_, main4);

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
  auto other   = Generate(generator_, cabinet_, genesis);
  auto main1   = Generate(generator_, cabinet_, other);
  auto main2   = Generate(generator_, cabinet_, main1);
  auto main3   = Generate(generator_, cabinet_, main2);
  auto main4   = Generate(generator_, cabinet_, main3);
  auto main5   = Generate(generator_, cabinet_, main4);

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

TEST_P(MainChainTests, CheckTipsWithStutter)
{
  auto genesis = generator_->Generate();

  auto chain1_1 = Generate(generator_, cabinet_, genesis, 2);
  // Generate 3 stutter blocks for chain 1
  auto chain1_2a = Generate(generator_, cabinet_, chain1_1, 3);
  auto chain1_2b = Generate(generator_, cabinet_, chain1_1, 3);
  auto chain1_2c = Generate(generator_, cabinet_, chain1_1, 3);
  auto chain1_3  = Generate(generator_, cabinet_, chain1_2a, 3);  // normal

  auto chain2_1 = Generate(generator_, cabinet_, genesis, 1);
  auto chain2_2 = Generate(generator_, cabinet_, chain2_1, 2);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_1));
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain2_1));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), chain1_1->body.hash);

  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_2a));
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain2_2));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), chain1_2a->body.hash);

  // Now at a second block with the same weight at the same height
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_2b));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), chain2_2->body.hash);

  // Should not accept more blocks from invalidated miner at same height
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_2c));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), chain2_2->body.hash);

  // Now receive block that builds on one of the tips removed
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_3));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), chain1_3->body.hash);
}

TEST_P(MainChainTests, StutterChain)
{
  auto genesis = generator_->Generate();

  // Build chain with stutter blocks at each height
  auto chain1_1a = Generate(generator_, cabinet_, genesis, 2);
  auto chain1_1b = Generate(generator_, cabinet_, genesis, 2);
  auto chain1_2a = Generate(generator_, cabinet_, chain1_1a, 3);
  auto chain1_2b = Generate(generator_, cabinet_, chain1_1a, 3);
  auto chain1_3a = Generate(generator_, cabinet_, chain1_2a, 3);
  auto chain1_3b = Generate(generator_, cabinet_, chain1_2a, 3);

  auto chain2_1 = Generate(generator_, cabinet_, genesis, 1);
  auto chain2_2 = Generate(generator_, cabinet_, chain2_1, 2);

  // Add blocks in chain 1a
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_1a));
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_2a));
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_3a));

  // Add chain 2
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain2_1));
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain2_2));

  // Heaviest is tip of 1a
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), chain1_3a->body.hash);

  // Add stutter blocks 1b
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_1b));
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_2b));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), chain1_3a->body.hash);

  // Add stutter tip and heaviest switches to chain 2
  ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain1_3b));
  ASSERT_EQ(chain_->GetHeaviestBlockHash(), chain2_2->body.hash);

  // Tips should now only contain genesis and heaviest tip
  auto                          tips       = chain_->GetTips();
  std::unordered_set<BlockHash> check_tips = {chain2_2->body.hash, genesis->body.hash};
  ASSERT_EQ(tips, check_tips);
}

TEST_P(MainChainTests, CheckReindexingOfTipsWithStutter)
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
  // Need to give blocks with the same block number different weights
  chain[0]  = generator_->Generate();
  chain[1]  = Generate(generator_, cabinet_, chain[0]);
  chain[2]  = Generate(generator_, cabinet_, chain[1]);
  chain[3]  = Generate(generator_, cabinet_, chain[2], 3);
  chain[4]  = Generate(generator_, cabinet_, chain[2], 4);
  chain[5]  = Generate(generator_, cabinet_, chain[3], 3);
  chain[6]  = Generate(generator_, cabinet_, chain[3], 4);
  chain[7]  = Generate(generator_, cabinet_, chain[4], 5);
  chain[8]  = Generate(generator_, cabinet_, chain[4], 6);
  chain[9]  = Generate(generator_, cabinet_, chain[5], 1);
  chain[10] = Generate(generator_, cabinet_, chain[5], 2);
  chain[11] = Generate(generator_, cabinet_, chain[6], 3);
  chain[12] = Generate(generator_, cabinet_, chain[6], 4);
  chain[13] = Generate(generator_, cabinet_, chain[7], 5);
  chain[14] = Generate(generator_, cabinet_, chain[7], 6);
  chain[15] = Generate(generator_, cabinet_, chain[8], 7);
  chain[16] = Generate(generator_, cabinet_, chain[8], 8);

  // add all the tips
  for (std::size_t i = 1; i < chain.size(); ++i)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*chain[i]));
  }

  // cache the state of the original tips
  auto const original_tips = chain_->GetTips();

  // Generate stutter blocks for some tips
  std::vector<BlockPtr>         stutter(3);
  std::unordered_set<BlockHash> stutter_tips;
  std::unordered_set<BlockHash> previous_to_stutter;
  stutter[0] = Generate(generator_, cabinet_, chain[5], 1);
  stutter[1] = Generate(generator_, cabinet_, chain[6], 4);
  stutter[2] = Generate(generator_, cabinet_, chain[8], 8);

  for (std::size_t i = 0; i < stutter.size(); ++i)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain_->AddBlock(*stutter[i]));
    stutter_tips.insert(stutter[i]->body.hash);
    previous_to_stutter.insert(stutter[i]->body.previous_hash);
  }

  // check tips contain no stutter blocks but have replaced them with their previous
  auto const new_tips                 = chain_->GetTips();
  auto const stutter_in_tips          = new_tips & stutter_tips;
  auto const stutter_previous_in_tips = new_tips & previous_to_stutter;
  ASSERT_TRUE(stutter_in_tips.empty());
  ASSERT_EQ(previous_to_stutter, stutter_previous_in_tips);
  ASSERT_TRUE(chain_->GetHeaviestBlockHash() == chain[15]->body.hash);

  // force the chain to index its tips again
  ASSERT_TRUE(chain_->ReindexTips());
  auto const new_reindexed_tips = chain_->GetTips();

  // compare the two sets and determine what if any nodes have been added or removed as compared
  // with original
  auto const new_missing_tips = new_tips - new_reindexed_tips;
  auto const new_added_tips   = new_reindexed_tips - new_tips;

  ASSERT_TRUE(new_missing_tips.empty());
  ASSERT_TRUE(new_added_tips.empty());
}

INSTANTIATE_TEST_CASE_P(ParamBased, MainChainTests,
                        ::testing::Values(MainChain::Mode::CREATE_PERSISTENT_DB,
                                          MainChain::Mode::IN_MEMORY_DB), );
