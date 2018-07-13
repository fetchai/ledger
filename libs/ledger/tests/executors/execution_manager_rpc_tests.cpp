#include "ledger/protocols/execution_manager_rpc_service.hpp"
#include "ledger/protocols/execution_manager_rpc_client.hpp"
#include "core/logger.hpp"

#include "mock_executor.hpp"
#include "test_block.hpp"
#include "block_configs.hpp"

#include <gmock/gmock.h>

#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>


class ExecutionManagerRpcTests : public ::testing::TestWithParam<BlockConfig> {
protected:
  using underlying_executor_type = FakeExecutor;
  using network_manager_type = std::unique_ptr<fetch::network::NetworkManager>;
  using shared_executor_type = std::shared_ptr<underlying_executor_type>;
  using executor_list_type = std::vector<shared_executor_type>;
  using underlying_manager_type = fetch::ledger::ExecutionManager;
  using executor_factory_type = underlying_manager_type::executor_factory_type;
  using underlying_client_type = fetch::ledger::ExecutionManagerRpcClient;
  using underlying_service_type = fetch::ledger::ExecutionManagerRpcService;
  using execution_manager_client_type = std::unique_ptr<underlying_client_type>;
  using execution_manager_service_type = std::unique_ptr<underlying_service_type>;

  void SetUp() override {
    static const uint16_t PORT = 9009;
    static const std::size_t NUM_NETWORK_THREADS = 2;

    auto const &config = GetParam();

    executors_.clear();

    network_manager_ = fetch::make_unique<fetch::network::NetworkManager>(NUM_NETWORK_THREADS);
    network_manager_->Start();

    // server
    service_ = fetch::make_unique<underlying_service_type>(PORT, *network_manager_, config.executors, [this]() {
      return CreateExecutor();
    });

    // client
    manager_ = fetch::make_unique<underlying_client_type>("127.0.0.1", PORT, *network_manager_);

    // wait for the client to connect before proceeding
    fetch::logger.Info("Connecting client to service...");
    while (!manager_->is_alive()) {
      std::this_thread::sleep_for(std::chrono::milliseconds{300});
    }
    fetch::logger.Info("Connecting client to service...complete");
  }

  void TearDown() override {
    network_manager_->Stop();

    manager_.reset();
    service_.reset();
    network_manager_.reset();
  }

  shared_executor_type CreateExecutor() {
    shared_executor_type executor = std::make_shared<underlying_executor_type>();
    executors_.push_back(executor);
    return executor;
  }

  bool WaitUntilExecutionComplete(std::size_t num_executions, std::size_t iterations = 4000) {
    bool success = false;

    for (std::size_t i = 0; i < iterations; ++i) {

      // the manager must be idle and have completed the required executions
      if (manager_->IsIdle() && (service_->completed_executions() >= num_executions)) {
        success = true;
        break;
      }

      std::this_thread::sleep_for(
        std::chrono::milliseconds{100}
      );
    }

    return success;
  }

  std::size_t GetNumExecutedTransaction() {
    std::size_t total = 0;

    for (auto const &executor : executors_) {
      total += executor->GetNumExecutions();
    }

    return total;
  }

  bool CheckForExecutionOrder() {
    using HistoryElement = underlying_executor_type::HistoryElement;
    using history_cache_type = underlying_executor_type::history_cache_type;

    bool success = false;

    // Step 1. Collect all the data from each of the executors
    history_cache_type history;
    history.reserve(GetNumExecutedTransaction());
    for (auto &exec : executors_) {
      exec->CollectHistory(history);
    }

    // Step 2. Sort the elements by timestamp
    std::sort(history.begin(),
              history.end(),
              [](HistoryElement const &a, HistoryElement const &b) {
                return a.timestamp < b.timestamp;
              });

    // Step 3. Check that the start time
    if (!history.empty()) {
      std::size_t current_slice = 0;

      success = true;
      for (std::size_t i = 1; i < history.size(); ++i) {
        auto const &current = history[i];

        if (current.slice == current_slice) {
          continue; // same slice
        } else if (current.slice > current_slice) {
          current_slice = current.slice;
          continue; // next slice
        } else {
          success = false;
          break;
        }
      }
    }

    return success;
  }

  network_manager_type network_manager_;
  execution_manager_client_type manager_;
  execution_manager_service_type service_;
  executor_list_type executors_;
};


TEST_P(ExecutionManagerRpcTests, CheckStuff) {
  BlockConfig const &config = GetParam();

  // generate a block with the desired lane and slice configuration
  auto block = TestBlock::Generate(config.lanes, config.slices);

  // execute the block
  ASSERT_TRUE(
    manager_->Execute(block.hash,
                      block.hash,
                      block.index,
                      block.map,
                      block.num_lanes,
                      block.num_slices)
  );

  // wait for the manager to become idle again
  ASSERT_TRUE(WaitUntilExecutionComplete(block.index.size()));
  ASSERT_EQ(GetNumExecutedTransaction(), block.index.size());
  ASSERT_TRUE(CheckForExecutionOrder());
}

INSTANTIATE_TEST_CASE_P(Param, ExecutionManagerRpcTests, ::testing::ValuesIn(BlockConfig::MAIN_SET),);
