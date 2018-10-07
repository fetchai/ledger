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

#include "core/logger.hpp"
#include "ledger/execution_manager.hpp"

#include "block_configs.hpp"
#include "mock_executor.hpp"
#include "test_block.hpp"

#include "mock_storage_unit.hpp"

#include <gmock/gmock.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>

using ::testing::_;

class ExecutionManagerStateTests : public ::testing::TestWithParam<BlockConfig>
{
public:
  using underlying_executor_type          = FakeExecutor;
  using shared_executor_type              = std::shared_ptr<underlying_executor_type>;
  using executor_list_type                = std::vector<shared_executor_type>;
  using underlying_execution_manager_type = fetch::ledger::ExecutionManager;
  using executor_factory_type   = underlying_execution_manager_type::executor_factory_type;
  using block_digest_type       = underlying_execution_manager_type::block_digest_type;
  using execution_manager_type  = std::shared_ptr<underlying_execution_manager_type>;
  using underlying_storage_type = MockStorageUnit;
  using storage_type            = std::shared_ptr<underlying_storage_type>;
  using clock_type              = std::chrono::high_resolution_clock;
  using status_type             = underlying_execution_manager_type::Status;

  void SetUp() override
  {
    auto const &config = GetParam();

    storage_.reset(new underlying_storage_type);
    executors_.clear();

    // create the manager
    manager_ = std::make_shared<underlying_execution_manager_type>(
        config.executors, storage_, [this]() { return CreateExecutor(); });
  }

  shared_executor_type CreateExecutor()
  {
    shared_executor_type executor = std::make_shared<underlying_executor_type>();
    executors_.push_back(executor);
    return executor;
  }

  bool WaitUntilManagerIsIdle(std::size_t num_executions, std::size_t num_iterations = 120)
  {
    bool success = false;

    for (std::size_t i = 0; i < num_iterations; ++i)
    {
      // exit condition
      if ((manager_->completed_executions() == num_executions) && manager_->IsIdle())
      {
        std::cerr << "WaitUntilManagerIsIdle - great success " << num_executions << std::endl;
        success = true;
        break;
      }

      // wait for a period of time
      std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }

    if (!success)
    {
      std::cerr << "Num Executions: " << manager_->completed_executions() << " vs. " << num_executions << " idle: " << manager_->IsActive() << " - " << manager_->IsIdle() << std::endl;
    }

    return success;
  }

  std::size_t GetNumExecutedTransaction()
  {
    std::size_t total = 0;

    for (auto const &executor : executors_)
    {
      total += executor->GetNumExecutions();
    }

    return total;
  }

  void ExecuteBlock(TestBlock &block, status_type expected_status = status_type::SCHEDULED)
  {
    ASSERT_TRUE(manager_->IsIdle());

    // determine the number of transactions that is expected from this execution
    std::size_t expected_completions = manager_->completed_executions();
    if (expected_status == status_type::SCHEDULED)
    {
      expected_completions += static_cast<std::size_t>(block.num_transactions);
    }

    // execute the block
    ASSERT_EQ(manager_->Execute(block.block), expected_status);

    // wait for the manager to become idle again
    ASSERT_TRUE(WaitUntilManagerIsIdle(expected_completions));
  }

  void AttachState()
  {
    for (auto &executor : executors_)
    {
      executor->SetStorageInterface(*storage_);
    }
  }

  storage_type           storage_;
  execution_manager_type manager_;
  executor_list_type     executors_;
};

std::ostream &operator<<(std::ostream &s, ExecutionManagerStateTests::status_type status)
{
  using fetch::ledger::ExecutionManager;

  switch (status)
  {
  case ExecutionManager::Status::COMPLETE:
    s << "Status::COMPLETE";
    break;
  case ExecutionManager::Status::SCHEDULED:
    s << "Status::COMPLETE";
    break;
  case ExecutionManager::Status::NOT_STARTED:
    s << "Status::COMPLETE";
    break;
  case ExecutionManager::Status::ALREADY_RUNNING:
    s << "Status::COMPLETE";
    break;
  case ExecutionManager::Status::NO_PARENT_BLOCK:
    s << "Status::COMPLETE";
    break;
  case ExecutionManager::Status::UNABLE_TO_PLAN:
    s << "Status::COMPLETE";
    break;
  default:
    s << "Status::UNKNOWN";
    break;
  }

  return s;
}

TEST_P(ExecutionManagerStateTests, CheckStateRollBack)
{
  BlockConfig const &config = GetParam();
  AttachState();  // so that we can see state updates

  std::cerr << "Config: " << config << std::endl;

  // generate a series of blocks in the pattern:
  //
  //         / block2
  // block1 -
  //         \ block3
  //
  auto block1 = TestBlock::Generate(config.log2_lanes, config.slices, __LINE__);
  auto block2 = TestBlock::Generate(config.log2_lanes, config.slices, __LINE__, block1.block.hash);
  auto block3 = TestBlock::Generate(config.log2_lanes, config.slices, __LINE__, block1.block.hash);

  // start the execution manager
  manager_->Start();

  {
    EXPECT_CALL(*storage_, Hash()).Times(1);
    EXPECT_CALL(*storage_, Set(_, _)).Times(block1.num_transactions);
    EXPECT_CALL(*storage_, Commit(_)).Times(1);

    ExecuteBlock(block1);
  }

  {
    EXPECT_CALL(*storage_, Hash()).Times(1);
    EXPECT_CALL(*storage_, Set(_, _)).Times(block2.num_transactions);
    EXPECT_CALL(*storage_, Commit(_)).Times(1);

    ExecuteBlock(block2);
  }

  auto const previous_hash = storage_->GetFake().Hash();

  {
    EXPECT_CALL(*storage_, Hash()).Times(1);
    EXPECT_CALL(*storage_, Commit(_)).Times(1);
    EXPECT_CALL(*storage_, Set(_, _)).Times(block3.num_transactions);
    EXPECT_CALL(*storage_, Revert(_)).Times(1);

    ExecuteBlock(block3);
  }

  {
    EXPECT_CALL(*storage_, Hash()).Times(0);
    EXPECT_CALL(*storage_, Set(_, _)).Times(0);
    EXPECT_CALL(*storage_, Commit(_)).Times(0);
    EXPECT_CALL(*storage_, Revert(_)).Times(1);

    ExecuteBlock(block2, status_type::COMPLETE);
  }

  auto const reapply_hash = storage_->GetFake().Hash();

  EXPECT_EQ(previous_hash, reapply_hash);

  // stop the ex
  manager_->Stop();
}

INSTANTIATE_TEST_CASE_P(Param, ExecutionManagerStateTests,
                        ::testing::ValuesIn(BlockConfig::REDUCED_SET), );
