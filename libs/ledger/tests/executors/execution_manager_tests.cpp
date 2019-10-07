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

#include "block_configs.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "mock_executor.hpp"
#include "mock_storage_unit.hpp"
#include "test_block.hpp"

#include "gmock/gmock.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <random>
#include <thread>
#include <vector>

namespace {

using namespace fetch::ledger;

class ExecutionManagerTests : public ::testing::TestWithParam<BlockConfig>
{
protected:
  using FakeExecutorPtr     = std::shared_ptr<FakeExecutor>;
  using FakeExecutorList    = std::vector<FakeExecutorPtr>;
  using ExecutorFactory     = ExecutionManager::ExecutorFactory;
  using ExecutionManagerPtr = std::shared_ptr<ExecutionManager>;
  using MockStorageUnitPtr  = std::shared_ptr<MockStorageUnit>;
  using Clock               = std::chrono::high_resolution_clock;
  using ScheduleStatus      = ExecutionManager::ScheduleStatus;
  using State               = ExecutionManager::State;

  void SetUp() override
  {
    BlockConfig const &config = GetParam();

    mock_storage_ = std::make_shared<MockStorageUnit>();
    executors_.clear();

    // create the manager
    manager_ =
        std::make_shared<ExecutionManager>(config.executors, config.log2_lanes, mock_storage_,
                                           [this]() { return CreateExecutor(); }, tx_status_cache_);
  }

  bool IsManagerIdle() const
  {
    return (State::IDLE == manager_->GetState());
  }

  FakeExecutorPtr CreateExecutor()
  {
    FakeExecutorPtr executor = std::make_shared<FakeExecutor>();
    executors_.push_back(executor);
    return executor;
  }

  // One day, this test will become more reliable. Until then, timeout at 60 seconds.
  bool WaitUntilExecutionComplete(std::size_t num_executions, std::size_t iterations = 600)
  {
    bool success = false;

    for (std::size_t i = 0; i < iterations; ++i)
    {

      // the manager must be idle and have completed the required executions
      if (IsManagerIdle() && (manager_->completed_executions() >= num_executions))
      {
        success = true;
        break;
      }

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

  bool CheckForExecutionOrder()
  {
    using HistoryElement      = FakeExecutor::HistoryElement;
    using HistoryElementCache = FakeExecutor::HistoryElementCache;

    bool success = false;

    // Step 1. Collect all the data from each of the executors
    HistoryElementCache history;
    history.reserve(GetNumExecutedTransaction());
    for (auto &exec : executors_)
    {
      exec->CollectHistory(history);
    }

    // Step 2. Sort the elements by timestamp
    std::sort(history.begin(), history.end(), [](HistoryElement const &a, HistoryElement const &b) {
      return a.timestamp < b.timestamp;
    });

    // Step 3. Check that the start time
    if (!history.empty())
    {
      std::size_t current_slice = 0;

      success = true;
      for (std::size_t i = 1; i < history.size(); ++i)
      {
        auto const &current = history[i];

        if (current.slice == current_slice)
        {
          continue;  // same slice
        }
        if (current.slice > current_slice)
        {
          current_slice = current.slice;
          continue;  // next slice
        }

        success = false;
        break;
      }
    }

    return success;
  }

  MockStorageUnitPtr              mock_storage_;
  ExecutionManagerPtr             manager_;
  FakeExecutorList                executors_;
  TransactionStatusCache::ShrdPtr tx_status_cache_{TransactionStatusCache::factory()};
};

TEST_P(ExecutionManagerTests, DISABLED_CheckIncrementalExecution)
{
  BlockConfig const &config = GetParam();

  // generate a block with the desired lane and slice configuration
  auto block = TestBlock::Generate(config.log2_lanes, config.slices, __LINE__);

  EXPECT_GT(block.num_transactions, 0);

  // start the execution manager
  manager_->Start();

  fetch::byte_array::ConstByteArray prev_hash;

  // execute the block
  ASSERT_EQ(manager_->Execute(block.block), ExecutionManager::ScheduleStatus::SCHEDULED);

  // wait for the manager to become idle again
  ASSERT_TRUE(WaitUntilExecutionComplete(static_cast<std::size_t>(block.num_transactions)));
  ASSERT_EQ(GetNumExecutedTransaction(), block.num_transactions);
  ASSERT_TRUE(CheckForExecutionOrder());

  // stop the ex
  manager_->Stop();
}

INSTANTIATE_TEST_CASE_P(Param, ExecutionManagerTests,
                        ::testing::ValuesIn(BlockConfig::REDUCED_SET), );

}  // namespace
