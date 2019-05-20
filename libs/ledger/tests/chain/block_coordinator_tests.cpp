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
#include "crypto/ecdsa.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chain/constants.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/v2/transaction_layout.hpp"
#include "ledger/testing/block_generator.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "testing/common_testing_functionality.hpp"

#include "crypto/sha256.hpp"
#include "fake_block_sink.hpp"
#include "mock_block_packer.hpp"
#include "mock_execution_manager.hpp"
#include "mock_storage_unit.hpp"

#include "gmock/gmock.h"
#include <iostream>
#include <memory>

using fetch::ledger::BlockCoordinator;
using fetch::ledger::MainChain;
using fetch::ledger::BlockStatus;
using fetch::ledger::Block;
using fetch::byte_array::ToBase64;
using fetch::ledger::GENESIS_DIGEST;
using fetch::crypto::ECDSASigner;
using fetch::ledger::testing::BlockGenerator;
using fetch::ledger::TransactionStatusCache;
using fetch::ledger::v2::TransactionLayout;
using fetch::ledger::v2::Address;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::StrictMock;

using BlockCoordinatorPtr = std::unique_ptr<BlockCoordinator>;
using MainChainPtr        = std::unique_ptr<MainChain>;
using ExecutionMgrPtr     = std::unique_ptr<MockExecutionManager>;
using StorageUnitPtr      = std::unique_ptr<MockStorageUnit>;
using BlockPackerPtr      = std::unique_ptr<MockBlockPacker>;
using BlockPtr            = std::shared_ptr<Block>;
using ScheduleStatus      = fetch::ledger::ExecutionManagerInterface::ScheduleStatus;
using BlockSinkPtr        = std::unique_ptr<FakeBlockSink>;
using TxCachePtr          = std::unique_ptr<TransactionStatusCache>;
using State               = fetch::ledger::BlockCoordinator::State;
using AddressPtr          = std::unique_ptr<Address>;

static constexpr char const *LOGGING_NAME = "BlockCoordinatorTests";
static constexpr std::size_t NUM_LANES    = 1;
static constexpr std::size_t NUM_SLICES   = 1;

class BlockCoordinatorTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    FETCH_UNUSED(LOGGING_NAME);

    block_generator_.Reset();

    // generate a public/private key pair
    ECDSASigner const signer{};

    address_           = std::make_unique<Address>(signer.identity());
    main_chain_        = std::make_unique<MainChain>(MainChain::Mode::IN_MEMORY_DB);
    storage_unit_      = std::make_unique<StrictMock<MockStorageUnit>>();
    execution_manager_ = std::make_unique<StrictMock<MockExecutionManager>>(storage_unit_->fake);
    packer_            = std::make_unique<StrictMock<MockBlockPacker>>();
    block_sink_        = std::make_unique<FakeBlockSink>();
    tx_status_         = std::make_unique<TransactionStatusCache>();
    block_coordinator_ = std::make_unique<BlockCoordinator>(
        *main_chain_, *execution_manager_, *storage_unit_, *packer_, *block_sink_, *tx_status_,
        signer.identity().identifier(), NUM_LANES, NUM_SLICES, 1u);

    block_coordinator_->SetBlockPeriod(std::chrono::seconds{10});
    block_coordinator_->EnableMining(true);
  }

  void TearDown() override
  {
    block_coordinator_.reset();
    tx_status_.reset();
    block_sink_.reset();
    packer_.reset();
    execution_manager_.reset();
    storage_unit_.reset();
    main_chain_.reset();
    address_.reset();
  }

  /**
   * Run the state machine
   */
  void Advance(uint64_t max_iterations = 50)
  {
    for (; max_iterations > 0; --max_iterations)
    {
      // run one step of the state machine
      block_coordinator_->GetRunnable().Execute();
    }
  }

  bool RemainsOn(State state, uint64_t iterations = 50)
  {
    bool success{true};

    auto &state_machine = block_coordinator_->GetStateMachine();

    for (uint64_t i = 0; success && i < iterations; ++i)
    {
      success = (state_machine.state() == state);

      state_machine.Execute();
    }

    return success;
  }

  /**
   * Run the state machine for one cycle
   *
   * @param starting_state The expected state before the state machine is run
   * @param final_state The expected state after the state machine has run
   */
  void Tick(State starting_state, State final_state)
  {
    auto const &state_machine = block_coordinator_->GetStateMachine();

    // match the current state of the machine
    ASSERT_EQ(starting_state, state_machine.state());

    // run one step of the state machine
    block_coordinator_->GetRunnable().Execute();

    ASSERT_EQ(final_state, state_machine.state());
  }

  /**
   * Run the state machine until it reaches the next state, or times out
   *
   * @param starting_state The expected state before the state machine is run
   * @param final_state The expected state after the state machine has run
   */
  void Tock(State starting_state, State final_state, uint64_t max_iterations = 50)
  {
    auto const &state_machine = block_coordinator_->GetStateMachine();

    // match the current state of the machine
    ASSERT_EQ(starting_state, state_machine.state());

    while (final_state != state_machine.state())
    {
      // run one step of the state machine
      block_coordinator_->GetRunnable().Execute();

      max_iterations--;

      if (max_iterations == 0)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Failed to advance state machine after ", max_iterations,
                        " iterations!");
        throw std::runtime_error("Failed test.");
      }
    }

    ASSERT_EQ(final_state, state_machine.state());
  }

  AddressPtr          address_;
  MainChainPtr        main_chain_;
  ExecutionMgrPtr     execution_manager_;
  StorageUnitPtr      storage_unit_;
  BlockPackerPtr      packer_;
  BlockSinkPtr        block_sink_;
  TxCachePtr          tx_status_;
  BlockCoordinatorPtr block_coordinator_;
  BlockGenerator      block_generator_{NUM_LANES, NUM_SLICES};
};

// useful when debugging
// static std::ostream& operator<<(std::ostream &stream, Block::Body const &block)
//{
//  stream << ToBase64(block.hash)
//         << " <- " << ToBase64(block.previous_hash);
//
//  return stream;
//}
//
// static std::ostream& operator<<(std::ostream &stream, BlockPtr const &block)
//{
//  if (block)
//  {
//    stream << block->body << " W: " << block->total_weight;
//  }
//  else
//  {
//    stream << "[empty]";
//  }
//
//  return stream;
//}

MATCHER(IsNewBlock, "")
{
  return arg.hash.empty();
}

MATCHER_P(IsBlock, block, "")
{
  return arg.hash == block->body.hash;
}

MATCHER_P(IsBlockFollowing, block, "")
{
  return arg.body.previous_hash == block->body.hash;
}

MATCHER_P(IsBlockBodyFollowing, block, "")
{
  return arg.previous_hash == block->body.hash;
}

TEST_F(BlockCoordinatorTests, CheckBasicInteraction)
{
  auto const genesis = block_generator_();

  // define how we expect the calls to be made
  {
    InSequence s;

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // pre block validation
    // none

    // schedule of the genesis block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(genesis)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(0));

    // reset
    // none

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // packer the new block
    EXPECT_CALL(*packer_, GenerateBlock(IsBlockFollowing(genesis), NUM_LANES, NUM_SLICES, _));

    // execute the block
    EXPECT_CALL(*execution_manager_, Execute(IsNewBlock()));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(1));
    EXPECT_CALL(*execution_manager_, SetLastProcessedBlock(_));

    // syncing back up
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
  }

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), GENESIS_DIGEST);

  Tick(State::RELOAD_STATE, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);

  // force the generation of a new block (normally done with a timer)
  block_coordinator_->SetBlockPeriod(
      std::chrono::minutes{2});  // time not important just needs to be long enough that the test
                                 // will not provoke a new block being generated
  block_coordinator_->TriggerBlockGeneration();

  Tick(State::SYNCHRONISED, State::PACK_NEW_BLOCK);
  Tick(State::PACK_NEW_BLOCK, State::EXECUTE_NEW_BLOCK);
  Tick(State::EXECUTE_NEW_BLOCK, State::WAIT_FOR_NEW_BLOCK_EXECUTION);
  Tick(State::WAIT_FOR_NEW_BLOCK_EXECUTION, State::WAIT_FOR_NEW_BLOCK_EXECUTION);
  Tick(State::WAIT_FOR_NEW_BLOCK_EXECUTION, State::PROOF_SEARCH);
  Tick(State::PROOF_SEARCH, State::TRANSMIT_BLOCK);
  Tick(State::TRANSMIT_BLOCK, State::RESET);

  ASSERT_NE(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);

  // the state machine should exit from the main loop
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
}

TEST_F(BlockCoordinatorTests, CheckLongBlockStartUp)
{
  auto genesis = block_generator_();
  auto b1      = block_generator_(genesis);
  auto b2      = block_generator_(b1);
  auto b3      = block_generator_(b2);
  auto b4      = block_generator_(b3);
  auto b5      = block_generator_(b4);

  // add all the blocks to the chain
  ASSERT_EQ(BlockStatus::ADDED, main_chain_->AddBlock(*b1));
  ASSERT_EQ(BlockStatus::ADDED, main_chain_->AddBlock(*b2));
  ASSERT_EQ(BlockStatus::ADDED, main_chain_->AddBlock(*b3));

  FETCH_LOG_INFO(LOGGING_NAME, "Genesis: ", ToBase64(genesis->body.hash), " <- ",
                 ToBase64(genesis->body.previous_hash));
  FETCH_LOG_INFO(LOGGING_NAME, "B1     : ", ToBase64(b1->body.hash), " <- ",
                 ToBase64(b1->body.previous_hash));
  FETCH_LOG_INFO(LOGGING_NAME, "B2     : ", ToBase64(b2->body.hash), " <- ",
                 ToBase64(b2->body.previous_hash));
  FETCH_LOG_INFO(LOGGING_NAME, "B3     : ", ToBase64(b3->body.hash), " <- ",
                 ToBase64(b3->body.previous_hash));
  FETCH_LOG_INFO(LOGGING_NAME, "B4     : ", ToBase64(b4->body.hash), " <- ",
                 ToBase64(b4->body.previous_hash));
  FETCH_LOG_INFO(LOGGING_NAME, "B5     : ", ToBase64(b5->body.hash), " <- ",
                 ToBase64(b5->body.previous_hash));

  // processing of genesis block
  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), GENESIS_DIGEST);

  {
    InSequence s;

    // reloading state
    EXPECT_CALL(*storage_unit_, RevertToHash(_, b3->body.block_number));

    // syncing - Genesis
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // pre block validation
    // none

    // schedule of the genesis block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(genesis)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(0));

    // syncing - B1
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
    EXPECT_CALL(*storage_unit_, HashExists(_, 0));
    EXPECT_CALL(*storage_unit_, RevertToHash(_, 0));

    // schedule of the next block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(b1)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(1));

    // syncing - B2
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
    EXPECT_CALL(*storage_unit_, HashExists(_, 1));
    EXPECT_CALL(*storage_unit_, RevertToHash(_, 1));

    // schedule of the next block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(b2)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(2));

    // syncing - B3
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
    EXPECT_CALL(*storage_unit_, HashExists(_, 2));
    EXPECT_CALL(*storage_unit_, RevertToHash(_, 2));

    // schedule of the next block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(b3)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(3));

    // syncing - moving to sync'ed state
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // --- Event: B4 added ---

    // syncing - B4
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
    EXPECT_CALL(*storage_unit_, HashExists(_, 3));
    EXPECT_CALL(*storage_unit_, RevertToHash(_, 3));

    // schedule of the next block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(b4)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(4));

    // syncing - moving to sync'ed state
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // --- Event: B5 added ---

    // syncing - B5
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
    EXPECT_CALL(*storage_unit_, HashExists(_, 4));
    EXPECT_CALL(*storage_unit_, RevertToHash(_, 4));

    // schedule of the next block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(b5)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(5));

    // syncing - moving to sync'ed state
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
  }

  Tick(State::RELOAD_STATE, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tock(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tock(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);

  // processing of B1 block
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tock(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), b1->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);

  // processing of B2 block
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tock(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), b2->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);

  // processing of B3 block
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tock(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), b3->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);

  // transition to synchronised state
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);

  // the state machine should rest in the state for a number of ticks
  for (std::size_t i = 0; i < 10; ++i)
  {
    Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  }

  // simulate B4 being recv'ed over the wire
  ASSERT_EQ(BlockStatus::ADDED, main_chain_->AddBlock(*b4));

  Tick(State::SYNCHRONISED, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tock(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), b4->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);

  // transition to synchronised state
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);

  // the state machine should rest in the state for a number of ticks
  for (std::size_t i = 0; i < 10; ++i)
  {
    Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  }

  // simulate B5 being recv'ed over the wire
  ASSERT_EQ(BlockStatus::ADDED, main_chain_->AddBlock(*b5));

  Tick(State::SYNCHRONISED, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tock(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), b5->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);

  // transition to synchronised state
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);

  // the state machine should rest in the state for a number of ticks
  for (std::size_t i = 0; i < 10; ++i)
  {
    Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  }
}

TEST_F(BlockCoordinatorTests, CheckInvalidBlockNumber)
{
  auto genesis = block_generator_();

  // define the call expectations
  {
    InSequence s;

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // pre block validation
    // none

    // schedule of the genesis block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(genesis)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(0));

    // reset
    // none

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
  }

  // processing of genesis block
  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), GENESIS_DIGEST);

  Tick(State::RELOAD_STATE, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  // create the bad block
  auto b1               = block_generator_(genesis);
  b1->body.block_number = 100;  // invalid block number
  b1->UpdateDigest();

  // main chain now rejects outright any blocks with invalid block numbers
  ASSERT_EQ(BlockStatus::INVALID, main_chain_->AddBlock(*b1));

  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);
}

TEST_F(BlockCoordinatorTests, CheckInvalidNumLanes)
{
  auto genesis = block_generator_();

  // define the call expectations
  {
    InSequence s;

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // pre block validation
    // none

    // schedule of the genesis block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(genesis)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(0));

    // reset
    // none

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // -- TEST CONFIG --

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
    EXPECT_CALL(*storage_unit_, HashExists(genesis->body.merkle_hash, ::testing::_));
    EXPECT_CALL(*storage_unit_, RevertToHash(genesis->body.merkle_hash, ::testing::_));

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
  }

  // processing of genesis block
  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), GENESIS_DIGEST);

  Tick(State::RELOAD_STATE, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  // create the bad block
  auto b1                 = block_generator_(genesis);
  b1->body.log2_num_lanes = 10;
  b1->UpdateDigest();

  ASSERT_EQ(BlockStatus::ADDED, main_chain_->AddBlock(*b1));

  Tick(State::SYNCHRONISED, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);
}

TEST_F(BlockCoordinatorTests, CheckInvalidNumSlices)
{
  auto genesis = block_generator_();

  // define the call expectations
  {
    InSequence s;

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // pre block validation
    // none

    // schedule of the genesis block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(genesis)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(0));
    // reset
    // none

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // -- TEST CONFIG --

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
    EXPECT_CALL(*storage_unit_, HashExists(genesis->body.merkle_hash, ::testing::_));
    EXPECT_CALL(*storage_unit_, RevertToHash(genesis->body.merkle_hash, ::testing::_));

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
  }

  // processing of genesis block
  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), GENESIS_DIGEST);

  Tick(State::RELOAD_STATE, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  // create the bad block
  auto b1 = block_generator_(genesis);
  b1->body.slices.resize(100);  // zero slices is always invalid
  b1->UpdateDigest();

  ASSERT_EQ(BlockStatus::ADDED, main_chain_->AddBlock(*b1));

  Tick(State::SYNCHRONISED, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);
}

TEST_F(BlockCoordinatorTests, CheckBlockMining)
{
  auto genesis = block_generator_();

  // define the call expectations
  {
    InSequence s;

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // pre block validation
    // none

    // schedule of the genesis block
    EXPECT_CALL(*execution_manager_, Execute(IsBlock(genesis)));

    // wait for the execution to complete
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());

    // post block validation
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(0));

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());

    // -- Event: Generate a block

    // block packing
    EXPECT_CALL(*packer_, GenerateBlock(IsBlockFollowing(genesis), _, _, _));

    // new block execution
    EXPECT_CALL(*execution_manager_, Execute(IsBlockBodyFollowing(genesis)));

    // waiting for block execution
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*execution_manager_, GetState());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*storage_unit_, Commit(1));

    // proof search
    EXPECT_CALL(*execution_manager_, SetLastProcessedBlock(_));

    // syncing
    EXPECT_CALL(*storage_unit_, LastCommitHash());
    EXPECT_CALL(*storage_unit_, CurrentHash());
    EXPECT_CALL(*execution_manager_, LastProcessedBlock());
  }

  // processing of genesis block
  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), GENESIS_DIGEST);

  Tick(State::RELOAD_STATE, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::PRE_EXEC_BLOCK_VALIDATION);
  Tick(State::PRE_EXEC_BLOCK_VALIDATION, State::WAIT_FOR_TRANSACTIONS);
  Tick(State::WAIT_FOR_TRANSACTIONS, State::SCHEDULE_BLOCK_EXECUTION);
  Tick(State::SCHEDULE_BLOCK_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::WAIT_FOR_EXECUTION);
  Tick(State::WAIT_FOR_EXECUTION, State::POST_EXEC_BLOCK_VALIDATION);

  ASSERT_EQ(execution_manager_->fake.LastProcessedBlock(), genesis->body.hash);

  Tick(State::POST_EXEC_BLOCK_VALIDATION, State::RESET);
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  Tick(State::SYNCHRONISED, State::SYNCHRONISED);

  // trigger the coordinator to try and make a block
  block_coordinator_->TriggerBlockGeneration();

  Tick(State::SYNCHRONISED, State::PACK_NEW_BLOCK);
  Tick(State::PACK_NEW_BLOCK, State::EXECUTE_NEW_BLOCK);
  Tick(State::EXECUTE_NEW_BLOCK, State::WAIT_FOR_NEW_BLOCK_EXECUTION);
  Tick(State::WAIT_FOR_NEW_BLOCK_EXECUTION, State::WAIT_FOR_NEW_BLOCK_EXECUTION);
  Tick(State::WAIT_FOR_NEW_BLOCK_EXECUTION, State::PROOF_SEARCH);
  Tick(State::PROOF_SEARCH, State::TRANSMIT_BLOCK);
  Tick(State::TRANSMIT_BLOCK, State::RESET);

  // ensure that the coordinator has actually made a block
  ASSERT_EQ(1u, block_sink_->queue().size());

  // ensure that the system goes back into the sync'ed state
  Tick(State::RESET, State::SYNCHRONISING);
  Tick(State::SYNCHRONISING, State::SYNCHRONISED);

  for (std::size_t i = 0; i < 20; ++i)
  {
    Tick(State::SYNCHRONISED, State::SYNCHRONISED);
  }
}

class NiceMockBlockCoordinatorTests : public BlockCoordinatorTests
{
protected:
  void SetUp() override
  {
    FETCH_UNUSED(LOGGING_NAME);

    block_generator_.Reset();

    // generate a public/private key pair
    ECDSASigner const signer{};

    clock_             = fetch::moment::CreateAdjustableClock("bc:deadline");
    main_chain_        = std::make_unique<MainChain>(MainChain::Mode::IN_MEMORY_DB);
    storage_unit_      = std::make_unique<NiceMock<MockStorageUnit>>();
    execution_manager_ = std::make_unique<NiceMock<MockExecutionManager>>(storage_unit_->fake);
    packer_            = std::make_unique<NiceMock<MockBlockPacker>>();
    block_sink_        = std::make_unique<FakeBlockSink>();
    tx_status_         = std::make_unique<TransactionStatusCache>();
    block_coordinator_ = std::make_unique<BlockCoordinator>(
        *main_chain_, *execution_manager_, *storage_unit_, *packer_, *block_sink_, *tx_status_,
        signer.identity().identifier(), NUM_LANES, NUM_SLICES, 1u);

    block_coordinator_->SetBlockPeriod(std::chrono::seconds{10});
    block_coordinator_->EnableMining(true);
  }

  fetch::moment::AdjustableClockPtr clock_;
};

TEST_F(NiceMockBlockCoordinatorTests, UnknownTransactionDoesNotBlockForever)
{
  TransactionLayout layout{*fetch::testing::GenerateUniqueHashes(1u).begin(), fetch::BitVector{}, 0,
                           0, 1000};

  auto genesis = block_generator_();
  auto b1      = block_generator_(genesis);

  // Fabricate unknown transaction
  b1->body.slices.begin()->push_back(layout);

  EXPECT_CALL(*storage_unit_, RevertToHash(_, 0));

  // syncing - Genesis
  EXPECT_CALL(*storage_unit_, LastCommitHash()).Times(AnyNumber());
  EXPECT_CALL(*storage_unit_, CurrentHash()).Times(AnyNumber());
  EXPECT_CALL(*execution_manager_, LastProcessedBlock()).Times(AnyNumber());

  Tock(State::RELOAD_STATE, State::SYNCHRONIZED);

  ASSERT_EQ(BlockStatus::ADDED, main_chain_->AddBlock(*b1));

  Advance();

  // Time out wait to request Tx from peers
  clock_->Advance(std::chrono::seconds(31u));

  ASSERT_TRUE(RemainsOn(State::WAIT_FOR_TRANSACTIONS));

  // Time out wait for Tx - block should be invalidated at this point
  clock_->Advance(std::chrono::seconds(31u));

  Tock(State::WAIT_FOR_TRANSACTIONS, State::SYNCHRONISED);
}
