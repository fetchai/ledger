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

#include "core/threading.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chain/consensus/consensus_miner_interface.hpp"
#include "ledger/chain/main_chain.hpp"
#include "metrics/metrics.hpp"
#include "miner/miner_interface.hpp"

#include <chrono>
#include <random>
#include <thread>

namespace fetch {
namespace ledger {

// TODO(issue 33): fine for now, but it would be more efficient if the block
//                 coordinator launched mining tasks
class MainChainMiner
{
public:
  using ConstByteArray          = byte_array::ConstByteArray;
  using BlockHash               = Block::Digest;
  using ConsensusMinerInterface = std::shared_ptr<fetch::chain::consensus::ConsensusMinerInterface>;
  using MinerInterface          = fetch::miner::MinerInterface;
  using BlockCompleteCallback   = std::function<void(Block const &)>;

  static constexpr char const *LOGGING_NAME    = "MainChainMiner";
  static constexpr uint32_t    BLOCK_PERIOD_MS = 5000;

  // Construction / Destruction
  MainChainMiner(std::size_t num_lanes, std::size_t num_slices, MainChain &mainChain,
                 chain::BlockCoordinator &block_coordinator, MinerInterface &miner,
                 ConsensusMinerInterface &consensus_miner, ConstByteArray miner_identity,
                 std::chrono::steady_clock::duration block_interval =
                     std::chrono::milliseconds{BLOCK_PERIOD_MS});
  MainChainMiner(MainChainMiner const &) = delete;
  MainChainMiner(MainChainMiner &&) = delete;
  ~MainChainMiner();

  void Start();
  void Stop();

  void OnBlockComplete(BlockCompleteCallback const &func);
  void SetConsensusMiner(ConsensusMinerInterface consensus_miner);

  // Operators
  MainChainMiner &operator=(MainChainMiner const &) = delete;
  MainChainMiner &operator=(MainChainMiner &&) = delete;

private:
  using Clock        = std::chrono::high_resolution_clock;
  using Timestamp    = Clock::time_point;
  using Milliseconds = std::chrono::milliseconds;

  void MinerThreadEntrypoint();

  std::atomic<bool> stop_{false};
  std::size_t       target_ = 8;
  std::size_t       num_lanes_;
  std::size_t       num_slices_;

  MainChain &                         main_chain_;
  chain::BlockCoordinator &           blockCoordinator_;
  MinerInterface &                    miner_;
  ConsensusMinerInterface             consensus_miner_;
  std::thread                         thread_;
  ConstByteArray                      miner_identity_;
  BlockCompleteCallback               on_block_complete_;
  std::chrono::steady_clock::duration block_interval_;
};

}  // namespace ledger
}  // namespace fetch
