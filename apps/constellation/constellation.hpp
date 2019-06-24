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

#include "core/feature_flags.hpp"
#include "core/reactor.hpp"
#include "http/module.hpp"
#include "http/server.hpp"
#include "ledger/block_sink_interface.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chain/consensus/consensus_miner_interface.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/consensus/entropy_generator_interface.hpp"
#include "ledger/consensus/stake_manager.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/genesis_loading/genesis_file_creator.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "ledger/storage_unit/lane_remote_control.hpp"
#include "ledger/storage_unit/storage_unit_bundled_service.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "ledger/transaction_processor.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "miner/basic_miner.hpp"
#include "network/muddle/muddle.hpp"
#include "network/p2pservice/manifest.hpp"
#include "network/p2pservice/p2p_service.hpp"
#include "network/p2pservice/p2ptrust_bayrank.hpp"

#include "ledger/dag/dag_interface.hpp"
#include "ledger/protocols/dag_service.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace fetch {

/**
 * Top level container for all components that are required to run a ledger instance
 */
class Constellation : public ledger::BlockSinkInterface
{
public:
  using Peer2PeerService = p2p::P2PService;
  using CertificatePtr   = Peer2PeerService::CertificatePtr;
  using UriList          = std::vector<network::Uri>;
  using Manifest         = network::Manifest;
  using NetworkMode      = ledger::MainChainRpcService::Mode;
  using FeatureFlags     = core::FeatureFlags;

  static constexpr uint32_t DEFAULT_BLOCK_DIFFICULTY = 6;

  struct Config
  {
    Manifest     manifest{};
    uint32_t     log2_num_lanes{0};
    uint32_t     num_slices{0};
    uint32_t     num_executors{0};
    std::string  interface_address{};
    std::string  db_prefix{};
    uint32_t     processor_threads{0};
    uint32_t     verification_threads{0};
    uint32_t     max_peers{0};
    uint32_t     transient_peers{0};
    uint32_t     block_interval_ms{0};
    uint32_t     block_difficulty{DEFAULT_BLOCK_DIFFICULTY};
    uint32_t     peers_update_cycle_ms{0};
    bool         disable_signing{false};
    bool         sign_broadcasts{false};
    bool         dump_state_file{false};
    bool         load_state_file{false};
    bool         proof_of_stake{false};
    NetworkMode  network_mode{NetworkMode::PUBLIC_NETWORK};
    FeatureFlags features{};

    uint32_t num_lanes() const
    {
      return 1u << log2_num_lanes;
    }
  };

  static constexpr char const *LOGGING_NAME = "constellation";

  // Construction / Destruction
  Constellation(CertificatePtr certificate, Config config);
  ~Constellation() override = default;

  void Run(UriList const &initial_peers, core::WeakRunnable bootstrap_monitor);
  void SignalStop();

protected:
  void OnBlock(ledger::Block const &block) override;

private:
  using Muddle                 = muddle::Muddle;
  using NetworkManager         = network::NetworkManager;
  using BlockPackingAlgorithm  = miner::BasicMiner;
  using BlockCoordinator       = ledger::BlockCoordinator;
  using MainChain              = ledger::MainChain;
  using MainChainRpcService    = ledger::MainChainRpcService;
  using MainChainRpcServicePtr = std::shared_ptr<MainChainRpcService>;
  using LaneServices           = ledger::StorageUnitBundledService;
  using StorageUnitClient      = ledger::StorageUnitClient;
  using LaneIndex              = ledger::LaneIdentity::lane_type;
  using StorageUnitClientPtr   = std::shared_ptr<StorageUnitClient>;
  using Flag                   = std::atomic<bool>;
  using ExecutionManager       = ledger::ExecutionManager;
  using ExecutionManagerPtr    = std::shared_ptr<ExecutionManager>;
  using LaneRemoteControl      = ledger::LaneRemoteControl;
  using HttpServer             = http::HTTPServer;
  using HttpModule             = http::HTTPModule;
  using HttpModulePtr          = std::shared_ptr<HttpModule>;
  using HttpModules            = std::vector<HttpModulePtr>;
  using TransactionProcessor   = ledger::TransactionProcessor;
  using TrustSystem            = p2p::P2PTrustBayRank<Muddle::Address>;
  using DAGPtr                 = std::shared_ptr<ledger::DAGInterface>;
  using DAGServicePtr          = std::shared_ptr<ledger::DAGService>;
  using SynergeticMinerPtr     = std::unique_ptr<ledger::SynergeticMinerInterface>;
  using NaiveSynergeticMiner   = ledger::NaiveSynergeticMiner;
  using StakeManagerPtr        = std::shared_ptr<ledger::StakeManager>;
  using EntropyPtr             = std::unique_ptr<ledger::EntropyGeneratorInterface>;

  using ShardConfigs  = ledger::ShardConfigs;
  using TxStatusCache = ledger::TransactionStatusCache;

  /// @name Configuration
  /// @{
  Flag         active_;           ///< Flag to control running of main thread
  Config       cfg_;              ///< The configuration
  uint16_t     p2p_port_;         ///< The port that the P2P interface is running from
  uint16_t     http_port_;        ///< The port of the HTTP server
  uint16_t     lane_port_start_;  ///< The starting port of all the lane services
  ShardConfigs shard_cfgs_;
  /// @}

  /// @name Network Orchestration
  /// @{
  core::Reactor    reactor_;
  NetworkManager   network_manager_;       ///< Top level network coordinator
  NetworkManager   http_network_manager_;  ///< A separate net. coordinator for the http service(s)
  Muddle           muddle_;                ///< The muddle networking service
  CertificatePtr   internal_identity_;
  Muddle           internal_muddle_;  ///< The muddle networking service
  TrustSystem      trust_;            ///< The trust subsystem
  Peer2PeerService p2p_;              ///< The main p2p networking stack
  /// @}

  /// @name Transaction and State Database shards
  /// @{
  TxStatusCache        tx_status_cache_;  ///< Cache of transaction status
  LaneServices         lane_services_;    ///< The lane services
  StorageUnitClientPtr storage_;          ///< The storage client to the lane services
  LaneRemoteControl    lane_control_;     ///< The lane control client for the lane services

  DAGPtr             dag_;
  DAGServicePtr      dag_service_;
  SynergeticMinerPtr synergetic_miner_;
  /// @}

  /// @name Staking
  /// @{
  EntropyPtr      entropy_;  ///< The entropy system
  StakeManagerPtr stake_;    ///< The stake system
  /// @}

  /// @name Block Processing
  /// @{
  ExecutionManagerPtr execution_manager_;  ///< The transaction execution manager
  /// @}

  /// @name Blockchain and Mining
  /// @[
  MainChain             chain_;              ///< The main block chain component
  BlockPackingAlgorithm block_packer_;       ///< The block packing / mining algorithm
  BlockCoordinator      block_coordinator_;  ///< The block execution coordinator
  /// @}

  /// @name Top Level Services
  /// @{
  MainChainRpcServicePtr main_chain_service_;  ///< Service for block transmission over the network
  TransactionProcessor   tx_processor_;        ///< The transaction entrypoint
  /// @}

  /// @name HTTP Server
  /// @{
  HttpServer  http_;          ///< The HTTP server
  HttpModules http_modules_;  ///< The set of modules currently configured
  /// @}
};

/**
 * Signal that constellation should attempt to shutdown gracefully
 */
inline void Constellation::SignalStop()
{
  active_ = false;
}

}  // namespace fetch
