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

#include "chain/constants.hpp"
#include "crypto/mcl_dkg.hpp"
#include "gtest/gtest.h"
#include "ledger/chain/main_chain.hpp"
#include "ledger/testing/block_generator.hpp"

using fetch::ledger::MainChain;
using fetch::ledger::BlockStatus;
using fetch::ledger::testing::BlockGenerator;
using fetch::Digest;

using BlockPtr = BlockGenerator::BlockPtr;

auto Generate(BlockGenerator &gen, BlockPtr genesis, std::size_t amount, std::size_t num_tx = 0)
{
  std::vector<BlockPtr> retVal(amount);
  for (auto &block : retVal)
  {
    genesis = block = gen.Generate(genesis, 1u, num_tx);
  }
  return retVal;
}

TEST(MainChainIntegrationTests, CheckRecoveryAfterCrash)
{
  fetch::crypto::mcl::details::MCLInitialiser();
  fetch::chain::InitialiseTestConstants();

  MainChain::Config cfg{false, 40, 1};

  // build a chain of blocks
  BlockGenerator gen{1, 2};
  auto const     genesis = gen.Generate();
  auto const     branch{Generate(gen, genesis, 200, 1)};

  Digest orig_heaviest_block_digest{};
  {
    MainChain chain1{MainChain::Mode::CREATE_PERSISTENT_DB, cfg};

    // add the branch of blocks to the chain
    for (auto const &blk : branch)
    {
      ASSERT_EQ(BlockStatus::ADDED, chain1.AddBlock(*blk));
    }

    // cache the heaviest
    auto const heaviest        = chain1.GetHeaviestBlock();
    orig_heaviest_block_digest = heaviest->hash;
  }

  MainChain chain2{MainChain::Mode::LOAD_PERSISTENT_DB, cfg};

  auto const recovered_heaviest = chain2.GetHeaviestBlock();

  // we expect that the heaviest block hashes do no match because the main chain has only
  // recovered all of its contents.
  EXPECT_NE(orig_heaviest_block_digest, recovered_heaviest->hash);
  EXPECT_EQ(190, recovered_heaviest->block_number);

  // should be able to add the remaining blocks again to the chain and have them being accepted.
  // This is important because the bloom filter needs to be kept in sync with the main chain
  for (std::size_t i = 190; i < branch.size(); ++i)
  {
    ASSERT_EQ(BlockStatus::ADDED, chain2.AddBlock(*branch.at(i)));
  }

  // finally we expect the two chains to be at the same end point
  EXPECT_EQ(orig_heaviest_block_digest, chain2.GetHeaviestBlockHash());
}
