#include "constellation.hpp"

namespace fetch {

Constellation::Constellation(uint16_t port_start, std::size_t num_executors, std::size_t num_lanes, std::string const &interface_address)
  : interface_address_{interface_address}
  , num_lanes_{static_cast<uint32_t>(num_lanes)}
  , p2p_port_{static_cast<uint16_t>(port_start + P2P_PORT_OFFSET)}
  , http_port_{static_cast<uint16_t>(port_start + HTTP_PORT_OFFSET)}
  , lane_port_start_{static_cast<uint16_t>(port_start + STORAGE_PORT_OFFSET)} {

  // determine how many threads the network manager will require
  std::size_t const num_network_threads =
    num_lanes * 2 + 10; // 2 := Lane/Storage Server, Lane/Storage Client 10 := provision for http and p2p

  // create the network manager
  network_manager_.reset(new fetch::network::NetworkManager(num_network_threads));
  network_manager_->Start(); // needs to be started

  // setup the storage service
  storage_service_.Setup("node_storage", num_lanes, lane_port_start_, *network_manager_, false);

  // create the aggregate storage client
  storage_.reset(new ledger::StorageUnitClient(*network_manager_));
  for (std::size_t i = 0; i < num_lanes; ++i) {
    storage_->AddLaneConnection<connection_type>(
      interface_address,
      static_cast<uint16_t>(lane_port_start_ + i)
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

  p2p_.reset(new p2p::P2PService(p2p_port_, *network_manager_));
  p2p_->Start();

  // define the list of HTTP modules to be used
  http_modules_ = {
    std::make_shared<ledger::ContractHttpInterface>(*storage_)
  };

  // create and register the HTTP modules
  http_.reset(new http::HTTPServer(http_port_, *network_manager_));
  for (auto const &module : http_modules_) {
    http_->AddModule(*module);
  }
}

void Constellation::Run(peer_list_type const &initial_peers) {
  using namespace std::chrono;
  using namespace std::this_thread;

  // make the initial p2p connections
  for (auto const &peer : initial_peers) {
    p2p_->Connect(peer.address(), peer.port());
  }

  // expose our own interface
  for (uint32_t i = 0; i < num_lanes_; ++i) {
    p2p_->AddLane(
      i,
      interface_address_,
      static_cast<uint16_t>(lane_port_start_ + i)
    );
  }

  // monitor loop
  while (active_) {
    logger.Info("Still alive...");
    sleep_for(seconds{5});
  }
}

} // namespace fetch
