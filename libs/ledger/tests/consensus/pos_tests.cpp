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
 * - Entropy is correct (notice entropy signature verification is turned off)
 * - Notarisations are correct
 *
 */

using namespace fetch;

using fetch::ledger::MainChain;
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
  using Digest                = Block::Hash;

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
  // populated fully) - note the prev hash here will be incorrect past 1.
  BlockPtr ValidNthBlock(uint64_t desired_block_number, uint64_t miner_index = 0)
  {
    BlockPtr ret     = std::make_shared<Block>();
    BlockPtr genesis = MainChain::CreateGenesisBlock();

    ret->block_number               = desired_block_number;
    ret->block_entropy.block_number = ret->block_number;
    ret->previous_hash              = genesis->hash;
    ret->miner_id                   = cabinet_[miner_index];
    ret->timestamp =
        GetTime(fetch::moment::GetClock("default", fetch::moment::ClockType::SYSTEM)) - 1;

    // Even though the entropy signature is not checked for this test, the thresholds etc. ARE
    // tested and need to be set appropriately
    if (desired_block_number == 1)
    {
      for (auto const &qual : qual_)
      {
        ret->block_entropy.qualified.insert(qual.identifier());
      }

      ret->block_entropy.HashSelf();

      for (auto const &key : cabinet_priv_keys_)
      {
        ret->block_entropy
            .confirmations[ret->block_entropy.ToQualIndex(key->identity().identifier())] =
            key->Sign(ret->block_entropy.digest);
      }

      assert(!ret->block_entropy.confirmations.empty());
    }
    else
    {
      ret->previous_hash = last_hash_used;
    }

    // This relies on generating blocks in order
    ret->previous_hash = chain_.GetHeaviestBlock()->hash;

    for (auto const &i : qual_)
    {
      ret->block_entropy.qualified.insert(i.identifier());
    }

    ret->weight = consensus_->GetBlockGenerationWeight(*ret, cabinet_[miner_index]);

    ret->UpdateDigest();
    ret->miner_signature = cabinet_priv_keys_[miner_index]->Sign(ret->hash);

    last_hash_used = ret->hash;
    chain_.AddBlock(*ret);

    return ret;
  }

  // Initialise these before the test
  fetch::crypto::mcl::details::MCLInitialiser init_before_others_{};

  // Run this before any of the tests.
  const int speedup_ = []() {
    fetch::chain::InitialiseTestConstants();
    return 0;
  }();

  ConsensusPtr          consensus_;
  Signers               cabinet_priv_keys_;
  Members               cabinet_;  // The identities that are in the cabinet
  Members               qual_;     // The identities that are in qual
  StakeManagerPtr       stake_;
  BeaconSetupServicePtr beacon_setup_;
  BeaconServicePtr      beacon_;
  Digest                last_hash_used;
  MainChain             chain_;
  StorageInterface      storage_;
  Identity              mining_identity_;
  uint64_t const        aeon_period_{10};
  uint64_t const        max_cabinet_size_{10};
  uint64_t const        block_interval_ms_{5000};
  NotarisationPtr       notarisation_;
};

TEST_F(ConsensusTests, test_valid_block)
{
  BlockPtr block = ValidNthBlock(1);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::YES);
}

TEST_F(ConsensusTests, test_invalid_block_number)
{
  BlockPtr block = ValidNthBlock(1);
  block->block_number++;
  block->block_entropy.block_number++;

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

  block->weight = block_by_other_miner->weight;
  block->UpdateDigest();
  block->miner_signature = cabinet_priv_keys_[0]->Sign(block->hash);

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
  Identity random_identity = signer->identity();

  block->miner_id = random_identity;
  block->UpdateDigest();

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_mismatched_cabinet_size)
{
  BlockPtr block = ValidNthBlock(1);

  consensus_->SetMaxCabinetSize(1);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_qual_too_small)
{
  BlockPtr block = ValidNthBlock(1);

  auto &entropy            = block->block_entropy;
  auto  confirmations_copy = entropy.confirmations;

  entropy.confirmations.clear();
  entropy.confirmations.insert(*confirmations_copy.begin());
  entropy.HashSelf();

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, test_unknown_qual_signed)
{
  BlockPtr block = ValidNthBlock(1);

  auto  signer  = std::make_shared<ECDSASigner>();
  auto &entropy = block->block_entropy;

  entropy.confirmations.erase(entropy.confirmations.begin());
  entropy.confirmations[entropy.ToQualIndex(signer->identity().identifier())] =
      signer->Sign(entropy.digest);

  // Since the update from the consensus up to N/3 failed confirmations are permitted. Since this
  // test only adds one this is permitted
  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::YES);
}

TEST_F(ConsensusTests, test_timestamp_ahead_in_time)
{
  BlockPtr block = ValidNthBlock(1);

  block->timestamp = block->timestamp + 10000;

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, non_qual_miner)
{
  BlockPtr block = ValidNthBlock(1);

  auto signer     = std::make_shared<ECDSASigner>();
  block->miner_id = signer->identity();
  block->weight   = 0;

  block->UpdateDigest();
  block->miner_signature = signer->Sign(block->hash);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, non_cabinet_qual)
{
  auto signer   = std::make_shared<ECDSASigner>();
  auto snapshot = std::make_shared<StakeSnapshot>();

  snapshot->UpdateStake(signer->identity(), 1);

  // All but the first
  for (std::size_t i = 1; i < cabinet_.size(); ++i)
  {
    snapshot->UpdateStake(cabinet_[i], 1);
  }
  consensus_->Reset(*snapshot, storage_);

  BlockPtr block = ValidNthBlock(1);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, incorrect_confirmation_sig)
{
  BlockPtr block                        = ValidNthBlock(1);
  block->block_entropy.confirmations[0] = block->block_entropy.confirmations[1];

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, loose_blocks_invalid)
{
  BlockPtr block = ValidNthBlock(2);

  block->previous_hash = block->hash;
  block->UpdateDigest();
  block->miner_signature = cabinet_priv_keys_[0]->Sign(block->hash);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}

TEST_F(ConsensusTests, not_aeon_beginning)
{
  BlockPtr block = ValidNthBlock(1);

  block->block_entropy.confirmations.clear();

  block->UpdateDigest();
  block->miner_signature = cabinet_priv_keys_[0]->Sign(block->hash);

  ASSERT_EQ(consensus_->ValidBlock(*block), ledger::ConsensusInterface::Status::NO);
}
