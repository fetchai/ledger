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
  using FakeExecutorPtr     = std::shared_ptr<FakeExecutor>;
  using FakeExecutorList    = std::vector<FakeExecutorPtr>;
  using ExecutionManager    = fetch::ledger::ExecutionManager;
  using ExecutorFactory     = ExecutionManager::ExecutorFactory;
  using BlockHash           = ExecutionManager::BlockHash;
  using ExecutionManagerPtr = std::shared_ptr<ExecutionManager>;
  using MockStorageUnitPtr  = std::shared_ptr<MockStorageUnit>;
  using Clock               = std::chrono::high_resolution_clock;
  using Status              = ExecutionManager::Status;

protected:
  void SetUp() override
  {
    BlockConfig const &config = GetParam();

    mock_storage_.reset(new MockStorageUnit);
    executors_.clear();

    // create the manager
    manager_ =
        std::make_shared<ExecutionManager>("exec_mgr_state_tests_", config.executors, mock_storage_,
                                           [this]() { return CreateExecutor(); });
  }

  FakeExecutorPtr CreateExecutor()
  {
    FakeExecutorPtr executor = std::make_shared<FakeExecutor>();
    executors_.push_back(executor);
    return executor;
  }

  bool WaitUntilManagerIsIdle(std::size_t num_executions, std::size_t num_iterations = 200)
  {
    bool success = false;

    for (std::size_t i = 0; i < num_iterations; ++i)
    {
      // exit condition
      if ((manager_->completed_executions() == num_executions) && manager_->IsIdle())
      {
        success = true;
        break;
      }

      // wait for a period of time
      std::this_thread::sleep_for(std::chrono::milliseconds{100});
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

  void ExecuteBlock(TestBlock &block, Status expected_status = Status::SCHEDULED)
  {
    ASSERT_TRUE(manager_->IsIdle());

    // determine the number of transactions that is expected from this execution
    std::size_t expected_completions = manager_->completed_executions();
    if (expected_status == Status::SCHEDULED)
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
      executor->SetStorageInterface(*mock_storage_);
    }
  }

  MockStorageUnitPtr  mock_storage_;
  ExecutionManagerPtr manager_;
  FakeExecutorList    executors_;
};

std::ostream &operator<<(std::ostream &s, ExecutionManagerStateTests::Status status)
{
  using fetch::ledger::ExecutionManager;

  switch (status)
  {
  case ExecutionManager::Status::COMPLETE:
    s << "Status::COMPLETE";
    break;
  case ExecutionManager::Status::SCHEDULED:
    s << "Status::SCHEDULED";
    break;
  case ExecutionManager::Status::NOT_STARTED:
    s << "Status::NOT_STARTED";
    break;
  case ExecutionManager::Status::ALREADY_RUNNING:
    s << "Status::ALREADY_RUNNING";
    break;
  case ExecutionManager::Status::NO_PARENT_BLOCK:
    s << "Status::NO_PARENT_BLOCK";
    break;
  case ExecutionManager::Status::UNABLE_TO_PLAN:
    s << "Status::UNABLE_TO_PLAN";
    break;
  default:
    s << "Status::UNKNOWN";
    break;
  }

  return s;
}

TEST_P(ExecutionManagerStateTests, DISABLED_CheckStateRollBack)
{
  BlockConfig const &config = GetParam();
  AttachState();  // so that we can see state updates

  // generate a series of blocks in the pattern:
  //
  //                    ┌──────────┐
  //                 ┌─▶│ Block 2  │
  //   ┌──────────┐  │  └──────────┘
  //   │ Block 1  │──┤
  //   └──────────┘  │  ┌──────────┐
  //                 └─▶│ Block 3  │
  //                    └──────────┘
  //
  auto block1 = TestBlock::Generate(config.log2_lanes, config.slices, __LINE__);
  auto block2 = TestBlock::Generate(config.log2_lanes, config.slices, __LINE__, block1.block.hash);
  auto block3 = TestBlock::Generate(config.log2_lanes, config.slices, __LINE__, block1.block.hash);

  // start the execution manager
  manager_->Start();

  {
    EXPECT_CALL(*mock_storage_, Hash()).Times(1);
    EXPECT_CALL(*mock_storage_, Set(_, _)).Times(block1.num_transactions);
    EXPECT_CALL(*mock_storage_, Commit(_)).Times(1);

    ExecuteBlock(block1);
  }

  {
    EXPECT_CALL(*mock_storage_, Hash()).Times(1);
    EXPECT_CALL(*mock_storage_, Set(_, _)).Times(block2.num_transactions);
    EXPECT_CALL(*mock_storage_, Commit(_)).Times(1);

    ExecuteBlock(block2);
  }

  auto const previous_hash = mock_storage_->GetFake().Hash();

  {
    EXPECT_CALL(*mock_storage_, Hash()).Times(1);
    EXPECT_CALL(*mock_storage_, Commit(_)).Times(1);
    EXPECT_CALL(*mock_storage_, Set(_, _)).Times(block3.num_transactions);
    EXPECT_CALL(*mock_storage_, Revert(_)).Times(1);

    ExecuteBlock(block3);
  }

  {
    EXPECT_CALL(*mock_storage_, Hash()).Times(0);
    EXPECT_CALL(*mock_storage_, Set(_, _)).Times(0);
    EXPECT_CALL(*mock_storage_, Commit(_)).Times(0);
    EXPECT_CALL(*mock_storage_, Revert(_)).Times(1);

    ExecuteBlock(block2, Status::COMPLETE);
  }

  auto const reapply_hash = mock_storage_->GetFake().Hash();

  EXPECT_EQ(previous_hash, reapply_hash);

  // stop the ex
  manager_->Stop();
}

INSTANTIATE_TEST_CASE_P(Param, ExecutionManagerStateTests,
                        ::testing::ValuesIn(BlockConfig::REDUCED_SET), );
