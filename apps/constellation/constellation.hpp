#ifndef FETCH_CONSTELLATION_HPP
#define FETCH_CONSTELLATION_HPP

#include "core/logger.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/management/network_manager.hpp"
#include "network/swarm/swarm_random.hpp"
#include "network/swarm/swarm_node.hpp"
#include "network/swarm/swarm_http_interface.hpp"
#include "network/swarm/swarm_agent_api_impl.hpp"
#include "ledger/executor.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/main_chain_node.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "ledger/storage_unit/storage_unit_bundled_service.hpp"
#include "python/worker/python_worker.hpp"

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
  using swarm_random_type = std::unique_ptr<swarm::SwarmRandom>;
  using swarm_node_type = std::shared_ptr<swarm::SwarmNode>;
  using swarm_http_module_type = std::unique_ptr<swarm::SwarmHttpModule>;
  using main_chain_node_type = std::shared_ptr<ledger::MainChainNode>;
  using swarm_worker_type = std::shared_ptr<swarm::PythonWorker>;
  using underlying_swarm_agent_type = fetch::swarm::SwarmAgentApiImpl<fetch::swarm::PythonWorker>;
  using swarm_agent_api_type = std::shared_ptr<underlying_swarm_agent_type>;
  using clock_type = std::chrono::high_resolution_clock;
  using timepoint_type = clock_type::time_point;
  using string_list_type = std::vector<std::string>;

  static constexpr uint32_t DEFAULT_MINING_TARGET = 10;
  static constexpr uint32_t DEFAULT_IDLE_SPEED = 2000;
  static constexpr uint16_t DEFAULT_PORT_START = 5000;
  static constexpr std::size_t DEFAULT_NUM_LANES = 8;
  static constexpr std::size_t DEFAULT_NUM_EXECUTORS = DEFAULT_NUM_LANES;

  static std::unique_ptr<Constellation> Create(uint32_t id,
                                               uint32_t max_peers,
                                               uint32_t mining_target = DEFAULT_MINING_TARGET,
                                               uint32_t idle_speed = DEFAULT_IDLE_SPEED,
                                               uint16_t port_start = DEFAULT_PORT_START,
                                               std::size_t num_executors = DEFAULT_NUM_EXECUTORS,
                                               std::size_t num_lanes = DEFAULT_NUM_LANES) {
    std::unique_ptr<Constellation> constellation{
      new Constellation{
        id,
        max_peers,
        mining_target,
        id, //chain_ident,
        idle_speed,
        port_start,
        num_executors,
        num_lanes
      }
    };
    return constellation;
  }

  explicit Constellation(uint32_t id, uint32_t max_peers, uint32_t mining_target,
    uint32_t chain_ident, uint32_t idle_speed,
    uint16_t port_start = DEFAULT_PORT_START,
                         std::size_t num_executors = DEFAULT_NUM_EXECUTORS,
                         std::size_t num_lanes = DEFAULT_NUM_LANES) {

    // work out the port mappings
    static constexpr uint16_t HTTP_PORT_OFFSET = 0;
    static constexpr uint16_t SWARM_PORT_OFFSET = 1;
    static constexpr uint16_t STORAGE_PORT_OFFSET = 2;

    uint16_t const swarm_port = port_start + SWARM_PORT_OFFSET;
    uint16_t const http_port = port_start + HTTP_PORT_OFFSET;
    uint16_t const storage_port_start = port_start + STORAGE_PORT_OFFSET;

    std::string const node_name = "node-" + std::to_string(id);
    std::string const swarm_host = "127.0.0.1:" + std::to_string(swarm_port);

    // determine how many threads the network manager will require
    std::size_t const num_network_threads = num_lanes * 2; // 2 := Storage Server, Storage Client

    // create the network manager
    network_manager_.reset(new fetch::network::NetworkManager(num_network_threads));
    network_manager_->Start();

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

    // create the swarm network node
    network_core_.reset(
      new network::NetworkNodeCore(
        *network_manager_,
        http_port,
        swarm_port
      )
    );

    swarm_rng_.reset(new swarm::SwarmRandom(id));

    swarm_node_.reset(
      new swarm::SwarmNode(
        network_core_,
        node_name,
        max_peers,
        swarm_host
      )
    );

    swarm_http_.reset(new swarm::SwarmHttpModule(swarm_node_));
    network_core_->AddModule(swarm_http_.get());

    main_chain_node_.reset(
      new ledger::MainChainNode(
        network_core_,
        id,
        mining_target,
        chain_ident
      )
    );

    swarm_worker_.reset(new swarm::PythonWorker);
    swarm_worker_->UseCore(network_core_);
    swarm_agent_api_.reset(
      new underlying_swarm_agent_type{
        swarm_worker_,
        swarm_host,
        idle_speed
      }
    );

    // TODO: (EJF) WTF???
    fetch::swarm::SwarmKarmaPeer::ToGetCurrentTime([](){ return time(0); });

    // make copies of shared pointers for the sake of the closures
    auto swarm_agent_api = swarm_agent_api_;
    auto network_core = network_core_;
    auto swarm_node = swarm_node_;
    auto chain_node = main_chain_node_;
    auto self = this;

    // TODO(kll) Move this setup code somewhere more sensible.
    swarm_agent_api->ToPing(
      [swarm_agent_api, network_core, swarm_node](fetch::swarm::SwarmAgentApi &unused, const std::string &host) {
        swarm_node->Post([swarm_agent_api, swarm_node, network_core, host]() {
          try {
            auto client = network_core->ConnectTo(host);
            if (!client) {
              swarm_agent_api->DoPingFailed(host);
              return;
            }

            auto newPeer = swarm_node->AskPeerForPeers(host, client);
            if (newPeer.length()) {
              if (!swarm_node->IsOwnLocation(newPeer)) {
                if (!swarm_node->IsExistingPeer(newPeer)) {
                  swarm_node->AddOrUpdate(host, 0);
                  swarm_agent_api->DoNewPeerDiscovered(newPeer);
                }
              }
              swarm_agent_api->DoPingSucceeded(host);
            } else {
              swarm_agent_api->DoPingFailed(host);
            }

          } catch (...) {
            swarm_agent_api->DoPingFailed(host);
          }
        });
      }
    );

    swarm_agent_api->ToDiscoverBlocks(
      [self, swarm_agent_api, swarm_node, chain_node, network_core](const std::string &host, uint32_t count) {
        swarm_node ->Post(
          [self, swarm_agent_api, network_core, chain_node, host, count]() {
            try {

              auto client = network_core->ConnectTo(host);
              if (!client) {
                swarm_agent_api->DoPingFailed(host);
               return;
              }

              auto promised = chain_node->RemoteGetHeaviestChain(count, client);
              if (promised.Wait()) {
                auto collection = promised.Get();
                if (collection.empty()) {
                 // must get at least genesis or this is an error case.
                 swarm_agent_api->DoPingFailed(host);
                }

                bool loose = false;
                std::string blockId;
                std::string prevHash;

                for(auto &block : collection) {
                  block.UpdateDigest();
                  chain_node->AddBlock(block);
                  prevHash = block.prevString();
                  loose = block.loose();
                  blockId = hashToBlockId(block.hash());
                  swarm_agent_api -> DoNewBlockIdFound(host, blockId);
                }

                if (loose) {
                  self->OnLooseBlock(host, prevHash);
                }

              } else {
                swarm_agent_api -> DoPingFailed(host);
              }

            } catch (...) {
              swarm_agent_api -> DoPingFailed(host);
            }
          }
        );
      }
    );

    swarm_agent_api->ToGetBlock(
      [self, swarm_agent_api, swarm_node, chain_node, network_core](const std::string &host, const std::string &block_id) {
        auto hashBytes = blockIdToHash(block_id);
        swarm_node->Post(
          [self, swarm_agent_api, swarm_node, chain_node, network_core, host, hashBytes, block_id]() {
            try {

              auto client = network_core->ConnectTo(host);
              if (!client) {
                swarm_agent_api->DoPingFailed(host);
                return;
              }

              auto promised = chain_node->RemoteGetHeader(hashBytes, client);
              if (promised.Wait()) {
                auto found = promised.Get().first;
                auto block = promised.Get().second;

                if (found) {
                  // add the block to the chainNode.
                  block.UpdateDigest();
                  auto newHash = block.hash();
                  auto newBlockId = hashToBlockId(newHash);

                  self->OnBlockSupplied(host, block.hashString());

                  chain_node->AddBlock(block);

                  if (block.loose()) {
                    self->OnLooseBlock(host, block.prevString());
                  }
                } else {
                  self->OnBlockNotSupplied(host, block_id);
                }
              } else {
                self->OnBlockNotSupplied(host, block_id);
              }
            } catch (...) {
              swarm_agent_api->DoPingFailed(host);
            }
          }
        );
      }
    );

    swarm_agent_api->ToGetKarma(
      [swarm_node](const std::string &host) {
        return swarm_node->GetKarma(host);
      }
    );

    swarm_agent_api->ToAddKarma(
      [swarm_node](const std::string &host, double amount) {
        swarm_node->AddOrUpdate(host, amount);
      }
    );

    swarm_agent_api->ToAddKarmaMax(
      [swarm_node](const std::string &host, double amount, double limit) {
        if (swarm_node->GetKarma(host) < limit) {
          swarm_node->AddOrUpdate(host, amount);
        }
      }
    );

    swarm_agent_api->ToGetPeers(
      [swarm_agent_api, swarm_node](uint32_t count, double minKarma) {
        auto karmaPeers = swarm_node->GetBestPeers(count, minKarma);

        std::list<std::string> results;
        for(auto &peer: karmaPeers) {
          results.push_back(peer.GetLocation().AsString());
        }

        if (results.empty()) {
          swarm_agent_api->DoPeerless();
        }

        return results;
      }
    );

    last_remote_new_block_time_ = clock_type::now();
  }

  void Run() {
    using namespace std::chrono;
    using namespace std::this_thread;

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

  static fetch::chain::MainChain::block_hash blockIdToHash(const std::string &id) {
    return fetch::byte_array::FromHex(id.c_str());
  }

  static std::string hashToBlockId(const fetch::chain::MainChain::block_hash &hash) {
    return static_cast<std::string>(fetch::byte_array::ToHex(hash));
  }

  void OnPingFailed(std::string const &host) {
    //say("PYCHAINNODE===> Ping failed to:", host)
    swarm_agent_api_->AddKarma(host, -5.);
    in_progress_.erase(host);
  }

  void OnPingSucceeded(std::string const &host) {
    swarm_agent_api_->AddKarmaMax(host, 10., 30.);
    in_progress_.erase(host);
  }

  int OnIdle() {
    auto good_peers = swarm_agent_api_->GetPeers(10, -0.5);

    auto const now = clock_type::now();
    auto const time_from_last_block = now - last_remote_new_block_time_;
    if (time_from_last_block > std::chrono::seconds{40}) {
      last_remote_new_block_time_ = now;
      in_progress_.clear();
    }

    // prune the good peer list
    std::remove_if(
      good_peers.begin(),
      good_peers.end(),
      [this](std::string const &peer) {

        if (in_progress_.find(peer) != in_progress_.end()) {
          return true;
        }

        if (std::find(introductions_.begin(), introductions_.end(), peer) != introductions_.end()) {
          return true;
        }

        return false;
      }
    );

    if (good_peers.empty()) {
      //say("quiet")
      return 100;
    }

    std::vector<std::string> hosts;

    // generate a set of weighted peers
    double total_weight = 0.0f;
    std::deque<std::tuple<std::string, double>> weighted_peers;
    std::transform(
      good_peers.begin(),
      good_peers.end(),
      std::back_inserter(weighted_peers),
      [this, &total_weight](std::string const &host) -> std::tuple<std::string, double> {
        double const weight = swarm_agent_api_->GetKarma(host);
        total_weight += weight;
        return {host, weight};
      }
    );

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<> dis;

    double weight = dis(rng) * total_weight;
    std::string weighted_peer;
    while ((weight >= 0.) && (!weighted_peers.empty())) {
      auto const &element = weighted_peers.front();
      weighted_peer = std::get<0>(element);
      weight -= std::get<1>(element);
      weighted_peers.pop_front();
    }

    if (!weighted_peers.empty()) {
      auto const &element = weighted_peers.front();
      hosts.push_back(std::get<0>(element));
    }

    std::uniform_int_distribution<int64_t> dist(0, static_cast<int64_t>(good_peers.size() - 1));
    int64_t const element_index = dist(rng);
    auto it = good_peers.begin();
    std::advance(it, element_index);
    hosts.push_back(*it);

    for (auto const &host : hosts) {
      if (in_progress_.find(host) == in_progress_.end()) {
        in_progress_.insert(host);
        swarm_agent_api_->DoPing(host);
        swarm_agent_api_->DoDiscoverBlocks(host, 3);
      } else {
        logger.Warn("Swarm: Sending ping to ", host);
      }
    }

    return 0;
  }

  void OnPeerless() {
    for (auto const &peer : initial_peer_list_) {
      swarm_agent_api_->DoPing(peer);
    }

    for (auto const &host : introductions_) {
      swarm_agent_api_->AddKarmaMax(host, 1000., 1000.);
    }
  }

  void OnNewPeerDiscovered(std::string const &host) {
    if (host == swarm_agent_api_->queryOwnLocation())
      return

    logger.Warn("Swarm: New peer detected");
  }

  void OnNewBlockIdFound(std::string const &host, std::string const &block_id) {
    logger.Warn("Swarm: Wow - new block found");

    swarm_agent_api_->AddKarmaMax(host, 2., 30.);
    last_remote_new_block_time_ = clock_type::now();
  }

  void OnBlockIdRepeated(std::string const &host, std::string const &block_id) {
    swarm_agent_api_->AddKarmaMax(host, 1., 10.);
  }

  void OnLooseBlock(std::string const &host, std::string const &block_id) {
    logger.Warn("Swarm: Loose found!");
    swarm_agent_api_->DoGetBlock(host, block_id);
  }

  void OnBlockSupplied(std::string const &host, std::string const &block_id) {
    logger.Warn("Swarm: Loose delivered");
  }

  void OnBlockNotSupplied(std::string const &host, std::string const &block_id) {
    logger.Warn("SwarmL Loose not delivered?");
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

  /// @name Swarm Networking Components
  /// @{
  network_core_type network_core_;
  swarm_random_type swarm_rng_;
  swarm_node_type swarm_node_;
  swarm_http_module_type swarm_http_;
  main_chain_node_type main_chain_node_;
  swarm_worker_type swarm_worker_;
  swarm_agent_api_type swarm_agent_api_;
  std::unordered_set<std::string> in_progress_;
  timepoint_type last_remote_new_block_time_;

  string_list_type initial_peer_list_;
  string_list_type introductions_;
  /// @}
};

} // namespace fetch

#endif //FETCH_CONSTELLATION_HPP
