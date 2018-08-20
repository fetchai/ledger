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

class ExecutionManagerTests : public ::testing::TestWithParam<BlockConfig>
{
protected:
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

  void SetUp() override
  {
    auto const &config = GetParam();

    storage_.reset(new underlying_storage_type);
    executors_.clear();

    // create the manager
    manager_ = std::make_shared<underlying_execution_manager_type>(
        config.executors, storage_, [this]() { return CreateExecutor(); });
  }

  void TearDown() override
  {
    manager_.reset();
    executors_.clear();
    storage_.reset();
  }

  shared_executor_type CreateExecutor()
  {
    shared_executor_type executor = std::make_shared<underlying_executor_type>();
    executors_.push_back(executor);
    return executor;
  }

  bool WaitUntilExecutionComplete(std::size_t num_executions, std::size_t iterations = 120)
  {
    bool success = false;

    for (std::size_t i = 0; i < iterations; ++i)
    {

      // the manager must be idle and have completed the required executions
      if (manager_->IsIdle() && (manager_->completed_executions() >= num_executions))
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
    using HistoryElement     = underlying_executor_type::HistoryElement;
    using history_cache_type = underlying_executor_type::history_cache_type;

    bool success = false;

    // Step 1. Collect all the data from each of the executors
    history_cache_type history;
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
        else if (current.slice > current_slice)
        {
          current_slice = current.slice;
          continue;  // next slice
        }
        else
        {
          success = false;
          break;
        }
      }
    }

    return success;
  }

  storage_type           storage_;
  execution_manager_type manager_;
  executor_list_type     executors_;
};

TEST_P(ExecutionManagerTests, CheckIncrementalExecution)
{
  BlockConfig const &config = GetParam();

  // generate a block with the desired lane and slice configuration
  auto block = TestBlock::Generate(config.log2_lanes, config.slices, __LINE__);

  fetch::logger.Info("Num transactions: ", block.num_transactions);
  EXPECT_GT(block.num_transactions, 0);

  // start the execution manager
  manager_->Start();

  EXPECT_CALL(*storage_, Hash()).Times(1);
  EXPECT_CALL(*storage_, Commit(_)).Times(1);

  fetch::byte_array::ConstByteArray prev_hash;

  // execute the block
  ASSERT_EQ(manager_->Execute(block.block), underlying_execution_manager_type::Status::SCHEDULED);

  // wait for the manager to become idle again
  ASSERT_TRUE(WaitUntilExecutionComplete(static_cast<std::size_t>(block.num_transactions)));
  ASSERT_EQ(GetNumExecutedTransaction(), block.num_transactions);
  ASSERT_TRUE(CheckForExecutionOrder());

  // stop the ex
  manager_->Stop();
}

INSTANTIATE_TEST_CASE_P(Param, ExecutionManagerTests, ::testing::ValuesIn(BlockConfig::MAIN_SET), );
