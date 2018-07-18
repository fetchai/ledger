#ifndef FETCH_CONSTELLATION_HPP
#define FETCH_CONSTELLATION_HPP

#include "core/logger.hpp"
#include "ledger/executor.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/main_chain_miner.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/main_chain_node.hpp"
#include "ledger/chaincode/contract_http_interface.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "ledger/storage_unit/storage_unit_bundled_service.hpp"
#include "network/p2pservice/p2p_service.hpp"
#include "network/peer.hpp"
#include "http/server.hpp"

#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <random>
#include <deque>
#include <tuple>

namespace fetch {

class Constellation {
public:
  using network_manager_type = std::shared_ptr<network::NetworkManager>;
  using executor_type = std::shared_ptr<ledger::Executor>;
  using executor_list_type = std::vector<executor_type>;
  using storage_client_type = std::shared_ptr<ledger::StorageUnitClient>;
  using connection_type = network::TCPClient;
  using execution_manager_type = std::shared_ptr<ledger::ExecutionManager>;
  using storage_service_type = ledger::StorageUnitBundledService;
  using flag_type = std::atomic<bool>;
  using network_core_type = std::shared_ptr<network::NetworkNodeCore>;
  using main_chain_node_type = std::shared_ptr<ledger::MainChainNode>;
  using clock_type = std::chrono::high_resolution_clock;
  using timepoint_type = clock_type::time_point;
  using string_list_type = std::vector<std::string>;
  using p2p_service_type = std::unique_ptr<p2p::P2PService>;
  using http_server_type = std::unique_ptr<http::HTTPServer>;
  using peer_list_type = std::vector<network::Peer>;
  using http_module_type = std::shared_ptr<http::HTTPModule>;
  using http_modules_type = std::vector<http_module_type>;

  static constexpr uint16_t P2P_PORT_OFFSET = 1;
  static constexpr uint16_t HTTP_PORT_OFFSET = 0;
  static constexpr uint16_t STORAGE_PORT_OFFSET = 10;
  static constexpr uint32_t DEFAULT_MINING_TARGET = 10;
  static constexpr uint32_t DEFAULT_IDLE_SPEED = 2000;
  static constexpr uint16_t DEFAULT_PORT_START = 5000;
  static constexpr std::size_t DEFAULT_NUM_LANES = 8;
  static constexpr std::size_t DEFAULT_NUM_EXECUTORS = DEFAULT_NUM_LANES;

  static std::unique_ptr<Constellation> Create(uint16_t port_start = DEFAULT_PORT_START,
                                               std::size_t num_executors = DEFAULT_NUM_EXECUTORS,
                                               std::size_t num_lanes = DEFAULT_NUM_LANES) {

    std::unique_ptr<Constellation> constellation{
      new Constellation{
        port_start,
        num_executors,
        num_lanes
      }
    };
    return constellation;
  }

  explicit Constellation(uint16_t port_start = DEFAULT_PORT_START,
                         std::size_t num_executors = DEFAULT_NUM_EXECUTORS,
                         std::size_t num_lanes = DEFAULT_NUM_LANES,
                         std::string const &interface_address = "127.0.0.1");

  void Run(peer_list_type const &initial_peers = peer_list_type{});

  executor_type CreateExecutor() {
    logger.Warn("Creating local executor...");
    return executor_type{new ledger::Executor(storage_)};
  }

private:

  flag_type active_{true};                      ///< Flag to control running of main thread
  std::string interface_address_;
  uint32_t num_lanes_;
  uint16_t p2p_port_;
  uint16_t http_port_;
  uint16_t lane_port_start_;

  network_manager_type network_manager_;        ///< Top level network coordinator

  /// @name Lane Storage Components
  /// @{
  storage_service_type storage_service_;        ///< The combination of all the lane services
  storage_client_type storage_;                 ///< The storage client
  /// @}

  /// @name Block Execution
  /// @{
  executor_list_type executors_;                ///< The list of transaction executors
  execution_manager_type execution_manager_;    ///< The execution manager
  /// @}

  /// @name Blockchain Components
  /// @{
  chain::MainChain main_chain_;                 ///< The main chain
  /// @}

  /// @name Blockchain Components
  /// @{
  chain::BlockCoordinator block_coordinator_;   ///< The block coordinator
  /// @}

  /// @name Blockchain Components
  /// @{
  chain::MainChainMiner main_chain_miner_;      ///< The main chain miner
  /// @}

  /// @name P2P Networking Components
  /// @{
  p2p_service_type p2p_;                        ///< The P2P networking component
  /// @}

  /// @name API Components
  /// @{
  http_server_type http_;                       ///< The HTTP interfaces server
  http_modules_type http_modules_;
  /// @}
};

} // namespace fetch

#endif //FETCH_CONSTELLATION_HPP
