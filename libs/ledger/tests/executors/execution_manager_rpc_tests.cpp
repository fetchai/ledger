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
#include "ledger/protocols/execution_manager_rpc_client.hpp"
#include "ledger/protocols/execution_manager_rpc_service.hpp"
#include "network/generics/atomic_inflight_counter.hpp"

#include "block_configs.hpp"
#include "fake_storage_unit.hpp"
#include "mock_executor.hpp"
#include "test_block.hpp"

#include <gmock/gmock.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>

class ExecutionManagerRpcTests : public ::testing::TestWithParam<BlockConfig>
{
protected:
  using NetworkManagerPtr             = std::unique_ptr<fetch::network::NetworkManager>;
  using FakeExecutorPtr               = std::shared_ptr<FakeExecutor>;
  using FakeExecutorList              = std::vector<FakeExecutorPtr>;
  using ExecutionManager              = fetch::ledger::ExecutionManager;
  using ExecutorFactory               = ExecutionManager::ExecutorFactory;
  using ExecutionManagerRpcClient     = fetch::ledger::ExecutionManagerRpcClient;
  using ExecutionManagerRpcService    = fetch::ledger::ExecutionManagerRpcService;
  using ExecutionManagerRpcClientPtr  = std::unique_ptr<ExecutionManagerRpcClient>;
  using ExecutionManagerRpcServicePtr = std::unique_ptr<ExecutionManagerRpcService>;
  using FakeStorageUnitPtr            = std::shared_ptr<FakeStorageUnit>;
  using ScheduleStatus                = ExecutionManager::ScheduleStatus;
  using State                         = ExecutionManager::State;

  static constexpr char const *LOGGING_NAME = "ExecutionManagerRpcTests";

  static bool WaitForLaneServersToStart()
  {
    using InFlightCounter =
        fetch::network::AtomicInFlightCounter<fetch::network::AtomicCounterName::TCP_PORT_STARTUP>;

    fetch::core::FutureTimepoint const deadline(std::chrono::seconds(30));

    return InFlightCounter::Wait(deadline);
  }

  void SetUp() override
  {
    static const uint16_t    PORT                = 9019;
    static const std::size_t NUM_NETWORK_THREADS = 2;

    BlockConfig const &config = GetParam();

    storage_.reset(new FakeStorageUnit);

    executors_.clear();

    network_manager_ =
        std::make_unique<fetch::network::NetworkManager>("NetMgr", NUM_NETWORK_THREADS);
    network_manager_->Start();

    // server
    service_ = std::make_unique<ExecutionManagerRpcService>(
        PORT, *network_manager_, config.executors, storage_, [this]() { return CreateExecutor(); });

    manager_ = std::make_unique<ExecutionManagerRpcClient>(*network_manager_);

    service_->Start();
    WaitForLaneServersToStart();

    FETCH_LOG_DEBUG(LOGGING_NAME, "Connecting client to service...complete");
  }

  void TearDown() override
  {
    service_->Stop();
    network_manager_->Stop();

    manager_.reset();
    service_.reset();
    network_manager_.reset();
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

  bool WaitUntilExecutionComplete(std::size_t num_executions, std::size_t iterations = 4000)
  {
    bool success = false;

    for (std::size_t i = 0; i < iterations; ++i)
    {

      // the manager must be idle and have completed the required executions
      if (IsManagerIdle() && (service_->completed_executions() >= num_executions))
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

  NetworkManagerPtr             network_manager_;
  ExecutionManagerRpcClientPtr  manager_;
  ExecutionManagerRpcServicePtr service_;
  FakeExecutorList              executors_;
  FakeStorageUnitPtr            storage_;
};

TEST_P(ExecutionManagerRpcTests, DISABLED_BlockExecution)
{
  BlockConfig const &config = GetParam();

  // generate a block with the desired lane and slice configuration
  auto block = TestBlock::Generate(config.log2_lanes, config.slices, __LINE__);

  // execute the block
  ASSERT_EQ(manager_->Execute(block.block), ExecutionManager::ScheduleStatus::SCHEDULED);

  // wait for the manager to become idle again
  ASSERT_TRUE(WaitUntilExecutionComplete(static_cast<std::size_t>(block.num_transactions)));
  ASSERT_EQ(GetNumExecutedTransaction(), block.num_transactions);
  ASSERT_TRUE(CheckForExecutionOrder());
}

INSTANTIATE_TEST_CASE_P(Param, ExecutionManagerRpcTests,
                        ::testing::ValuesIn(BlockConfig::MAIN_SET), );
