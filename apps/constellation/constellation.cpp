#include "constellation.hpp"

namespace fetch {

Constellation::Constellation(uint16_t port_start, std::size_t num_executors, std::size_t num_lanes) {

  // work out the port mappings
  static constexpr uint16_t P2P_PORT_OFFSET = 1;
  static constexpr uint16_t HTTP_PORT_OFFSET = 0;
  static constexpr uint16_t STORAGE_PORT_OFFSET = 10;

  uint16_t const p2p_port = port_start + P2P_PORT_OFFSET;
  uint16_t const http_port = port_start + HTTP_PORT_OFFSET;
  uint16_t const storage_port_start = port_start + STORAGE_PORT_OFFSET;

  // determine how many threads the network manager will require
  std::size_t const num_network_threads =
    num_lanes * 2 + 10; // 2 := Lane/Storage Server, Lane/Storage Client 10 := provision for http and p2p

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

void Constellation::Run(peer_list_type const &initial_peers) {
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

} // namespace fetch
