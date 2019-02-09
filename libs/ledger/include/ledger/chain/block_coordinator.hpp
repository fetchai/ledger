#pragma once
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

#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/execution_manager.hpp"
#include "miner/miner_interface.hpp"

#include <atomic>
#include <thread>

namespace fetch {
namespace ledger {

class BlockCoordinator
{
public:
  using Block     = ledger::Block;
  using MainChain = ledger::MainChain;

  static constexpr char const *LOGGING_NAME = "BlockCoordinator";

  // Construction / Destruction
  BlockCoordinator(MainChain &chain, ledger::ExecutionManagerInterface &execution_manager);
  BlockCoordinator(BlockCoordinator const &) = delete;
  BlockCoordinator(BlockCoordinator &&)      = delete;
  ~BlockCoordinator();

  /// @name Service Control
  /// @{
  void Start();
  void Stop();
  /// @}

  void AddBlock(Block &block);

  // Operators
  BlockCoordinator &operator=(BlockCoordinator const &) = delete;
  BlockCoordinator &operator=(BlockCoordinator &&) = delete;

private:
  enum class State
  {
    QUERY_EXECUTION_STATUS,
    GET_NEXT_BLOCK,
    EXECUTE_BLOCK,
  };

  using Mutex         = fetch::mutex::Mutex;
  using BlockBodyPtr  = std::shared_ptr<Block::Body>;
  using PendingBlocks = std::deque<BlockBodyPtr>;
  using PendingStack  = std::vector<BlockBodyPtr>;
  using Flag          = std::atomic<bool>;

  /// @name Monitor State
  /// @{
  void Monitor();

  void OnQueryExecutionStatus();
  void OnGetNextBlock();
  void OnExecuteBlock();
  /// @}

  /// @name Internal
  /// @{
  MainChain &                        chain_;              ///< Ref to system chain
  ledger::ExecutionManagerInterface &execution_manager_;  ///< Ref to system execution manager
  Flag                               stop_{false};        ///< Flag to signal stop of monitor
  std::thread                        thread_;             ///< The monitor thread
  /// @}

  /// @name Pending Block Queue
  /// @{
  Mutex         pending_blocks_mutex_{__LINE__, __FILE__};
  PendingBlocks pending_blocks_{};  /// The pending blocks weighting to be executed
  /// @}

  /// @name Monitor State
  /// @{
  State        state_{State::QUERY_EXECUTION_STATUS};  ///< The current state of the monitor
  BlockBodyPtr current_block_;                         ///< The pointer to the current block
  PendingStack pending_stack_;  ///< The stack of pending blocks to be executed
  /// @}
};

}  // namespace ledger
}  // namespace fetch
