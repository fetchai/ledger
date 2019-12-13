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

#include "crypto/ecdsa.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/consensus/consensus.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/storage_unit/fake_storage_unit.hpp"
#include "moment/deadline_timer.hpp"

#include "gtest/gtest.h"

/**
 * These tests are designed to check that high level consensus checks are being enforced.
 * It does not aim to check:
 * - Anything that requires execution (TXs are there, lanes, slices, merkle hash)
 * - Entropy is correct
 * - Notarisations are correct
 *
 */

using namespace fetch;

using fetch::ledger::MainChain;
using fetch::moment::DeadlineTimer;
using fetch::ledger::Block;
using fetch::crypto::ECDSASigner;

class ConsensusTests : public ::testing::Test
{
protected:
  using Consensus             = ledger::Consensus;
  using ConsensusPtr          = std::shared_ptr<Consensus>;
  using Signers               = std::vector<std::shared_ptr<ECDSASigner>>;
  using Members               = Consensus::WeightedQual;
  using StakeManager          = ledger::StakeManager;
  using StakeManagerPtr       = Consensus::StakeManagerPtr;
  using MainChain             = ledger::MainChain;
  using MainChainPtr          = std::shared_ptr<ledger::MainChain>;
  using StorageInterface      = ledger::FakeStorageUnit;
  using BeaconSetupServicePtr = Consensus::BeaconSetupServicePtr;
  using BeaconServicePtr      = Consensus::BeaconServicePtr;
  using Identity              = Consensus::Identity;
  using BlockEntropy          = Consensus::BlockEntropy;
  using NotarisationPtr       = Consensus::NotarisationPtr;
  using Block                 = ledger::Block;
  using BlockPtr              = std::shared_ptr<Block>;
  using StakeSnapshot         = fetch::ledger::StakeSnapshot;
  using ConstByteArray        = byte_array::ConstByteArray;

  void SetUp() override
  {
    auto snapshot = std::make_shared<StakeSnapshot>();

    // Set up our initial cabinet as stakers
    for (std::size_t i = 0; i < max_cabinet_size_; ++i)
    {
      auto     signer   = std::make_shared<ECDSASigner>();
      Identity identity = Identity{signer->identity().identifier()};
      cabinet_priv_keys_.push_back(signer);
      cabinet_.emplace_back(identity);
      qual_.emplace_back(identity);
      snapshot->UpdateStake(identity, 1);
    }

    mining_identity_ = cabinet_[0];

    stake_        = std::make_shared<StakeManager>();
    beacon_setup_ = nullptr;
    beacon_       = nullptr;
    notarisation_ = nullptr;

    consensus_ = std::make_shared<Consensus>(stake_, beacon_setup_, beacon_, chain_, storage_,
                                             mining_identity_, aeon_period_, max_cabinet_size_,
                                             block_interval_ms_, notarisation_);

    consensus_->Reset(*snapshot, storage_);
  }

  // return a valid first block after genesis (entropy not
  // populated fully)
  BlockPtr ValidNthBlock(uint64_t desired_block_number, uint64_t miner_index = 0)
  {
    BlockPtr ret     = std::make_shared<Block>();
    BlockPtr genesis = MainChain::CreateGenesisBlock();

    ret->block_number               = desired_block_number;
    ret->block_entropy.block_number = ret->block_number;
    ret->previous_hash              = genesis->hash;
    ret->miner_id                   = cabinet_[miner_index];
    ret->timestamp = GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM));

    // Need to 'trick' the entropy as appearing as an aeon beginning to avoid
    // having to create it
    if (desired_block_number == 1)
    {
      ret->block_entropy.confirmations[ConstByteArray("test")];
      assert(!ret->block_entropy.confirmations.empty());
    }
    else
    {
      // This relies on generating blocks in order
      ret->previous_hash = chain_.GetHeaviestBlock()->hash;
    }

    for (auto const &i : qual_)
    {
      ret->block_entropy.qualified.insert(i.identifier());
    }

    ret->weight = consensus_->GetBlockGenerationWeight(*ret, chain::Address{mining_identity_});

    ret->UpdateDigest();
    ret->miner_signature = cabinet_priv_keys_[miner_index]->Sign(ret->hash);

    chain_.AddBlock(*ret);

    return ret;
  }

  fetch::crypto::mcl::details::MCLInitialiser init_before_others_{};
  ConsensusPtr                                consensus_;
  Signers                                     cabinet_priv_keys_;
  Members                                     cabinet_;  // The identities that are in the cabinet
  Members                                     qual_;     // The identities that are in qual
  StakeManagerPtr                             stake_;
  BeaconSetupServicePtr                       beacon_setup_;
  BeaconServicePtr                            beacon_;
  MainChain                                   chain_;
  StorageInterface                            storage_;
  Identity                                    mining_identity_;
  uint64_t const                              aeon_period_{10};
  uint64_t const                              max_cabinet_size_{10};
  uint64_t const                              block_interval_ms_{5000};
  NotarisationPtr                             notarisation_;
};

TEST_F(ConsensusTests, test_valid_block)
{
  BlockPtr block = ValidNthBlock(1);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::YES);
}

TEST_F(ConsensusTests, test_loose_block)
{
  BlockPtr block = ValidNthBlock(1);
  block->block_number++;

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_entropy_block_number_mismatch)
{
  BlockPtr block = ValidNthBlock(1);
  block->block_entropy.block_number++;

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_hash_empty)
{
  BlockPtr block = ValidNthBlock(1);
  block->hash    = "";

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_not_an_aeon)
{
  BlockPtr block = ValidNthBlock(1);
  block->block_entropy.confirmations.clear();

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_zero_weight)
{
  BlockPtr block = ValidNthBlock(1);
  block->weight  = 0;

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_wrong_weight)
{
  BlockPtr block = ValidNthBlock(1);
  block->weight++;

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_stolen_weight)
{
  BlockPtr block                = ValidNthBlock(1);
  BlockPtr block_by_other_miner = ValidNthBlock(1, 1);

  block->weight   = block_by_other_miner->weight;
  block->UpdateDigest();
  ret->miner_signature = cabinet_priv_keys_[0]->Sign(block->hash);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_timestamp_too_early)
{
  BlockPtr block = ValidNthBlock(1);
  block          = ValidNthBlock(2);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_timestamp_before_previous)
{
  BlockPtr block = ValidNthBlock(1);
  block          = ValidNthBlock(2);

  block->timestamp = 0;

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_not_member_of_qual)
{
  BlockPtr block = ValidNthBlock(1);

  auto     signer          = std::make_shared<ECDSASigner>();
  Identity random_identity = Identity{signer->identity().identifier()};

  block->miner_id = random_identity;
  block->UpdateDigest();

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_mismatched_digest)
{
  BlockPtr block = ValidNthBlock(1);

  block->block_entropy.confirmations[ConstByteArray("a")];
  block->block_entropy.confirmations[ConstByteArray("b")];

  block->UpdateDigest();

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_mismatched_cabinet_size)
{
  BlockPtr block = ValidNthBlock(1);

  consensus_->SetMaxCabinetSize(1);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}
