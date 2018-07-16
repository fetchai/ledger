#ifndef FETCH_CONSTELLATION_HPP
#define FETCH_CONSTELLATION_HPP

#include "core/logger.hpp"
#include "ledger/executor.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/main_chain_node.hpp"
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
                         std::size_t num_lanes = DEFAULT_NUM_LANES) {

    // work out the port mappings
    static constexpr uint16_t P2P_PORT_OFFSET = 1;
    static constexpr uint16_t HTTP_PORT_OFFSET = 0;
    static constexpr uint16_t STORAGE_PORT_OFFSET = 10;

    uint16_t const p2p_port = port_start + P2P_PORT_OFFSET;
    uint16_t const http_port = port_start + HTTP_PORT_OFFSET;
    uint16_t const storage_port_start = port_start + STORAGE_PORT_OFFSET;

    // determine how many threads the network manager will require
    std::size_t const num_network_threads = num_lanes * 2 + 10; // 2 := Lane/Storage Server, Lane/Storage Client 10 := provision for http and p2p

    // create the network manager
    network_manager_.reset(new fetch::network::NetworkManager(num_network_threads));
    network_manager_->Start(); // needs to be started

    // setup the storage service
    storage_service_.Setup("node_storage", num_lanes, storage_port_start, *network_manager_, false);

    // create the aggregate storage client
    storage_.reset(new ledger::StorageUnitClient(*network_manager_));
    for (std::size_t i = 0; i < num_lanes; ++i) {
      storage_->AddLaneConnection<connection_type>(
        "127.0.0.1",
        static_cast<uint16_t>(storage_port_start + i)
      );
    }

    // create the execution manager (and its executors)
    execution_manager_.reset(
      new ledger::ExecutionManager(num_executors, storage_, [this]() {
        auto executor = CreateExecutor();
        executors_.push_back(executor);
        return executor;
      })
    );

    p2p_.reset(new p2p::P2PService(p2p_port, *network_manager_));
    p2p_->Start();

    http_.reset(new http::HTTPServer(http_port, *network_manager_));
  }

  void Run(peer_list_type const &initial_peers = peer_list_type{}) {
    using namespace std::chrono;
    using namespace std::this_thread;

    // make the initial p2p connections
    for (auto const &peer : initial_peers) {
      p2p_->Connect(peer.address(), peer.port());
    }

    // monitor loop
    while (active_) {
      logger.Info("Still alive...");
      sleep_for(seconds{5});
    }
  }

  executor_type CreateExecutor() {
    logger.Warn("Creating local executor...");
    return executor_type{new ledger::Executor(storage_)};
  }

private:

  flag_type active_{true};                      ///< Flag to control running of main thread
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

  /// @name P2P Networking Components
  /// @{
  p2p_service_type p2p_;                        ///< The P2P networking component
  /// @}

  /// @name API Components
  /// @{
  http_server_type http_;                       ///< The HTTP interfaces server
  /// @}
};

} // namespace fetch

#endif //FETCH_CONSTELLATION_HPP
