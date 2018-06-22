#ifndef SWARM_CONTROLLER_HPP
#define SWARM_CONTROLLER_HPP
#include "service/client.hpp"
#include "service/publication_feed.hpp"

#include "protocols/fetch_protocols.hpp"
#include "protocols/swarm/node_details.hpp"

#include "chain/block_generator.hpp"
#include "protocols/chain_keeper/chain_manager.hpp"
#include "protocols/swarm/serializers.hpp"

#include <atomic>
#include <unordered_set>

namespace fetch {
namespace protocols {

class ChainController {
 public:
  typedef crypto::CallableFNV hasher_type;
  // Block defs
  typedef fetch::chain::consensus::ProofOfWork proof_type;
  typedef fetch::chain::BlockBody block_body_type;
  typedef typename proof_type::header_type block_header_type;
  typedef fetch::chain::BasicBlock<proof_type, fetch::crypto::SHA256>
      block_type;
  typedef std::shared_ptr<block_type> shared_block_type;

  typedef std::unordered_map<block_header_type, shared_block_type, hasher_type>
      chain_map_type;

  ChainController() {
    grouping_parameter_ = 1;

    block_body_type genesis_body;
    block_type genesis_block;

    genesis_body.previous_hash = "genesis";
    genesis_body.group_parameter = 1;

    genesis_block.SetBody(genesis_body);

    genesis_block.set_block_number(0);

    PushBlock(genesis_block);
  }

  std::vector<block_type> GetLatestBlocks() {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);

    if (latest_blocks_.size() > 25) {
      std::vector<block_type> ret;
      std::size_t n = latest_blocks_.size() -
                      std::min(std::size_t(25), latest_blocks_.size());
      for (; n < latest_blocks_.size(); ++n) {
        ret.push_back(latest_blocks_[n]);
      }

      std::swap(ret, latest_blocks_);

      return ret;
    }

    return latest_blocks_;
  }

  block_type GetNextBlock() {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    block_body_type body;
    block_type block;

    block_mutex_.lock();

    body.previous_hash = head_->header();
    body.group_parameter = grouping_parameter_;

    block_generator_.set_group_count(grouping_parameter_);
    block_generator_.GenerateBlock(body,
                                   std::size_t(1.5 * grouping_parameter_));
    double tw = head_->total_weight();

    block_mutex_.unlock();
    block.SetBody(body);

    auto &p = block.proof();
    p.SetTarget(1);
    ++p;
    p();
    double work = fetch::math::Log(p.digest());  // TODO: Check formula
    block.set_total_weight(tw + work);
    block.set_weight(work);

    return block;
  }

  void PushBlock(block_type block) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    // Only record blocks that are new
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);

    if (chains_.find(block.header()) != chains_.end()) {
      fetch::logger.Debug("Block already exists");

      return;
    }

    TODO("Trim latest blocks");
    latest_blocks_.push_back(block);

    for (auto &tx : block.body().transactions) {
      block_generator_.PushTransactionSummary(tx);
    }

    block_header_type header = block.header();

    chain_map_type::iterator pit;

    pit = chains_.find(block.body().previous_hash);

    if (pit != chains_.end()) {
      block.set_block_number(pit->second->block_number() + 1);
      block.set_previous(pit->second);
      block.set_is_loose(pit->second->is_loose());
    } else {
      // First block added is always genesis and by definition not loose
      block.set_is_loose(chains_.size() != 0);
    }

    auto shared_block = std::make_shared<block_type>(block);
    chains_[block.header()] = shared_block;

    // TODO: Set next
    if (block.is_loose()) {
      fetch::logger.Debug("Found loose block");
      TODO("Handle loose blocks!");

    } else if (!head_) {
      head_ = shared_block;
      block_generator_.SwitchBranch(head_);
    } else if ((block.total_weight() >= head_->total_weight())) {
      head_ = shared_block;
      block_generator_.SwitchBranch(head_);
    }
  }

  std::size_t block_count() const {
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);
    return chains_.size();
  }

  void AddBulkBlocks(std::vector<block_type> const &new_blocks) {
    for (auto block : new_blocks) {
      PushBlock(block);
    }
  }

  void AddBulkSummaries(
      std::vector<chain::TransactionSummary> const &summaries) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    std::lock_guard<fetch::mutex::Mutex> lock(block_mutex_);
    for (auto &s : summaries) {
      block_generator_.PushTransactionSummary(s);
    }
  }

  void SetGroupParameter(uint32_t total_groups) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    grouping_parameter_ = total_groups;
  }

  void with_blocks_do(std::function<void(ChainManager::shared_block_type,
                                         ChainManager::chain_map_type const &)>
                          fnc) const {
    block_mutex_.lock();
    fnc(head_, chains_);
    block_mutex_.unlock();
  }

  void with_blocks_do(std::function<void(ChainManager::shared_block_type,
                                         ChainManager::chain_map_type &)>
                          fnc) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    block_mutex_.lock();
    fnc(head_, chains_);
    block_mutex_.unlock();
  }

  /*

*/

 private:
  mutable fetch::mutex::Mutex block_mutex_;
  chain::BlockGenerator block_generator_;

  chain_map_type chains_;
  shared_block_type head_;
  std::vector<block_type> latest_blocks_;

  std::atomic<uint32_t> grouping_parameter_;
};

class SwarmController : public ChainController,
                        public fetch::service::HasPublicationFeed {
 public:
  typedef fetch::service::ServiceClient<fetch::network::TCPClient> client_type;
  typedef std::shared_ptr<client_type> client_shared_ptr_type;

  SwarmController(uint64_t const &protocol,
                  network::ThreadManager *thread_manager,
                  SharedNodeDetails &details)
      : protocol_(protocol),
        thread_manager_(thread_manager),
        details_(details),
        suggestion_mutex_(__LINE__, __FILE__),
        peers_mutex_(__LINE__, __FILE__),
        chain_keeper_mutex_(__LINE__, __FILE__),
        grouping_parameter_(1) {
    LOG_STACK_TRACE_POINT;

    // Do not modify details_ here as it is not yet initialized.
  }

  uint64_t Ping() {
    LOG_STACK_TRACE_POINT;

    std::cout << "PING" << std::endl;

    return 1337;
  }

  NodeDetails Hello(uint64_t client, NodeDetails details) {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(client_details_mutex_);
    client_details_[client] = details;
    //    SendConnectivityDetailsToGroups(details);    --- TODO Figure out why
    //    this dead locks
    return details_.details();
  }

  std::vector<NodeDetails> SuggestPeers() {
    LOG_STACK_TRACE_POINT;

    if (need_more_connections()) {
      RequestPeerConnections(details_.details());
    }

    std::lock_guard<fetch::mutex::Mutex> lock(suggestion_mutex_);

    return peers_with_few_followers_;
  }

  void RequestPeerConnections(NodeDetails details) {
    LOG_STACK_TRACE_POINT;

    NodeDetails me = details_.details();

    suggestion_mutex_.lock();
    if (already_seen_.find(std::string(details.public_key)) ==
        already_seen_.end()) {
      std::cout << "Discovered " << details.public_key << std::endl;
      peers_with_few_followers_.push_back(details);
      already_seen_.insert(std::string(details.public_key));
      this->Publish(SwarmFeed::FEED_REQUEST_CONNECTIONS, details);

      for (auto &client : peers_) {
        client->Call(protocol_, SwarmRPC::REQUEST_PEER_CONNECTIONS, details);
      }

    } else {
      std::cout << "Ignored " << details.public_key << std::endl;
    }

    suggestion_mutex_.unlock();
  }

  void EnoughPeerConnections(NodeDetails details) {
    LOG_STACK_TRACE_POINT;

    suggestion_mutex_.lock();
    bool found = false;
    auto it = peers_with_few_followers_.end();
    while (it != peers_with_few_followers_.begin()) {
      --it;
      if ((*it).public_key == details.public_key) {
        found = true;
        peers_with_few_followers_.erase(it);
      }
    }

    if (found) {
      this->Publish(SwarmFeed::FEED_ENOUGH_CONNECTIONS, details);
    }
    suggestion_mutex_.unlock();
  }

  std::string GetAddress(uint64_t client) {
    if (request_ip_) return request_ip_(client);

    return "unknown";
  }

  void IncreaseGroupingParameter() {
    LOG_STACK_TRACE_POINT;

    chain_keeper_mutex_.lock();
    uint32_t n = grouping_parameter_ << 1;

    fetch::logger.Debug("Increasing group parameter to ", n);

    struct IDClientPair {
      std::size_t index;
      client_shared_ptr_type client;
    };

    std::map<std::size_t, std::vector<IDClientPair> > group_org;

    // Computing new grouping assignment
    for (std::size_t i = 0; i < n; ++i) {
      group_org[i] = std::vector<IDClientPair>();
    }

    for (std::size_t i = 0; i < chain_keepers_.size(); ++i) {
      auto &details = chain_keepers_details_[i];
      auto &client = chain_keepers_[i];
      uint32_t s = details.group;
      group_org[s].push_back({i, client});
    }

    for (std::size_t i = 0; i < grouping_parameter_; ++i) {
      std::vector<IDClientPair> &vec = group_org[i];
      if (vec.size() < 2) {
        TODO("Throw error - cannot perform grouping without more nodes");
        chain_keeper_mutex_.unlock();
        return;
      }

      std::size_t bucket2 = i + grouping_parameter_;
      std::vector<IDClientPair> &nvec = group_org[bucket2];

      std::size_t q = vec.size() >> 1;  // Diving group into two groups
      for (; q != 0; --q) {
        nvec.push_back(vec.back());
        vec.pop_back();
      }
    }

    // Assigning group values
    for (std::size_t i = 0; i < n; ++i) {
      fetch::logger.Debug("Updating group nodes in group ", i);
      auto &vec = group_org[i];
      for (auto &c : vec) {
        chain_keepers_details_[c.index].group = group_type(i);
        c.client->Call(FetchProtocols::CHAIN_KEEPER,
                       ChainKeeperRPC::SET_GROUP_NUMBER, uint32_t(i),
                       uint32_t(n));
      }
    }

    grouping_parameter_ = n;
    chain_keeper_mutex_.unlock();
  }

  void SetGroupParameter(uint32_t total_groups) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    grouping_parameter_ = total_groups;

    ChainController::SetGroupParameter(total_groups);
  }

  uint32_t GetGroupingParameter() { return grouping_parameter_; }

  ////////////////////////
  // Not service protocol
  client_shared_ptr_type ConnectChainKeeper(
      byte_array::ConstByteArray const &host, uint16_t const &port) {
    LOG_STACK_TRACE_POINT;

    fetch::logger.Debug("Connecting to group ", host, ":", port);

    chain_keeper_mutex_.lock();
    client_shared_ptr_type client =
        std::make_shared<client_type>(host, port, thread_manager_);
    chain_keeper_mutex_.unlock();

    std::this_thread::sleep_for(
        std::chrono::milliseconds(500));  // TODO: Make variable

    EntryPoint ep = client
                        ->Call(fetch::protocols::FetchProtocols::CHAIN_KEEPER,
                               ChainKeeperRPC::HELLO, host)
                        .As<EntryPoint>();

    fetch::logger.Highlight("Before Add");

    with_node_details([](NodeDetails const &det) {
      for (auto &e : det.entry_points) {
        fetch::logger.Debug("  --- ", e.host, ":", e.port);
      }

    });
    details_.AddEntryPoint(ep);

    fetch::logger.Highlight("After Add");
    with_node_details([](NodeDetails const &det) {
      for (auto &e : det.entry_points) {
        fetch::logger.Debug("  --- ", e.host, ":", e.port, " is group ",
                            (e.configuration & EntryPoint::NODE_CHAIN_KEEPER));
      }

    });

    chain_keeper_mutex_.lock();
    chain_keepers_.push_back(client);
    chain_keepers_details_.push_back(ep);
    fetch::logger.Debug("Total group count = ", chain_keepers_.size());
    chain_keeper_mutex_.unlock();

    return client;
  }

  void SetClientIPCallback(std::function<std::string(uint64_t)> request_ip) {
    request_ip_ = request_ip;
  }

  client_shared_ptr_type Connect(byte_array::ConstByteArray const &host,
                                 uint16_t const &port) {
    LOG_STACK_TRACE_POINT;

    using namespace fetch::service;
    fetch::logger.Debug("Connecting to server on ", host, " ", port);
    client_shared_ptr_type client =
        std::make_shared<client_type>(host, port, thread_manager_);

    std::this_thread::sleep_for(
        std::chrono::milliseconds(500));  // TODO: Connection feedback

    fetch::logger.Debug("Pinging server to confirm connection.");
    auto ping_promise = client->Call(protocol_, SwarmRPC::PING);

    if (!ping_promise.Wait(2000)) {
      fetch::logger.Error("Client not repsonding - hanging up!");
      return nullptr;
    }

    fetch::logger.Debug("Subscribing to feeds.");
    client->Subscribe(protocol_, SwarmFeed::FEED_REQUEST_CONNECTIONS,
                      new service::Function<void(NodeDetails)>(
                          [this](NodeDetails const &details) {
                            SwarmController::RequestPeerConnections(details);
                          }));

    client->Subscribe(
        protocol_, SwarmFeed::FEED_ENOUGH_CONNECTIONS,
        new Function<void(NodeDetails)>([this](NodeDetails const &details) {
          SwarmController::EnoughPeerConnections(details);
        }));

    client->Subscribe(
        protocol_, SwarmFeed::FEED_ANNOUNCE_NEW_COMER,
        new Function<void(NodeDetails)>([](NodeDetails const &details) {
          std::cout << "TODO: figure out what to do here" << std::endl;
        }));

    uint64_t ping = uint64_t(ping_promise);

    fetch::logger.Debug("Waiting for ping.");
    if (ping == 1337) {
      fetch::logger.Info("Successfully got PONG");

      peers_mutex_.lock();
      peers_.push_back(client);
      peers_mutex_.unlock();

      service::Promise ip_promise =
          client->Call(protocol_, SwarmRPC::WHATS_MY_IP);
      std::string own_ip = ip_promise.As<std::string>();
      fetch::logger.Info("Node host is ", own_ip);

      // Creating
      protocols::EntryPoint e;
      e.host = own_ip;
      e.group = 0;
      e.port = details_.default_port();
      e.http_port = details_.default_http_port();
      e.configuration = EntryPoint::NODE_SWARM;
      details_.AddEntryPoint(e);

      //      if(need_more_connections() ) {
      auto mydetails = details_.details();
      service::Promise details_promise =
          client->Call(protocol_, SwarmRPC::HELLO, mydetails);
      client->Call(protocol_, SwarmRPC::REQUEST_PEER_CONNECTIONS, mydetails);

      // TODO: add mutex
      NodeDetails server_details = details_promise.As<NodeDetails>();
      std::cout << "Setting details for server with handle: "
                << client->handle() << std::endl;

      if (server_details.entry_points.size() == 0) {
        protocols::EntryPoint e2;
        e2.host = client->Address();
        e2.group = 0;
        e2.port = server_details.default_port;
        e2.http_port = server_details.default_http_port;
        e2.configuration = EntryPoint::NODE_SWARM;
        server_details.entry_points.push_back(e2);
      }

      //      SendConnectivityDetailsToGroups(server_details);
      peers_mutex_.lock();
      server_details_[client->handle()] = server_details;
      peers_mutex_.unlock();
    } else {
      fetch::logger.Error("Server gave wrong response - hanging up!");
      return nullptr;
    }

    return client;
  }

  bool need_more_connections() const { return true; }

  void Bootstrap(byte_array::ConstByteArray const &host, uint16_t const &port) {
    LOG_STACK_TRACE_POINT;

    // TODO: Check that this node qualifies for bootstrapping
    std::cout << " - bootstrapping " << host << " " << port << std::endl;
    auto client = Connect(host, port);
    if (!client) {
      fetch::logger.Error("Failed in bootstrapping!");
      return;
    }

    auto peer_promise = client->Call(protocol_, SwarmRPC::SUGGEST_PEERS);

    std::vector<NodeDetails> others =
        peer_promise.As<std::vector<NodeDetails> >();

    for (auto &o : others) {
      std::cout << "Consider connecting to " << o.public_key << std::endl;
      if (already_seen_.find(std::string(o.public_key)) ==
          already_seen_.end()) {
        already_seen_.insert(std::string(o.public_key));
        peers_with_few_followers_.push_back(o);
      }
    }
  }

  void with_shard_details_do(
      std::function<void(std::vector<EntryPoint> &)> fnc) {
    chain_keeper_mutex_.lock();
    fnc(chain_keepers_details_);

    chain_keeper_mutex_.unlock();
  }

  void with_shards_do(
      std::function<void(std::vector<client_shared_ptr_type> const &,
                         std::vector<EntryPoint> &)>
          fnc) {
    chain_keeper_mutex_.lock();
    fnc(chain_keepers_, chain_keepers_details_);
    chain_keeper_mutex_.unlock();
  }

  void with_shards_do(
      std::function<void(std::vector<client_shared_ptr_type> const &)> fnc) {
    chain_keeper_mutex_.lock();
    fnc(chain_keepers_);
    chain_keeper_mutex_.unlock();
  }

  void with_suggestions_do(
      std::function<void(std::vector<NodeDetails> &)> fnc) {
    std::lock_guard<fetch::mutex::Mutex> lock(suggestion_mutex_);
    fnc(peers_with_few_followers_);
  }

  void with_client_details_do(
      std::function<void(std::map<uint64_t, NodeDetails> const &)> fnc) {
    std::lock_guard<fetch::mutex::Mutex> lock(client_details_mutex_);

    fnc(client_details_);
  }

  void with_peers_do(
      std::function<void(std::vector<client_shared_ptr_type>)> fnc) {
    std::lock_guard<fetch::mutex::Mutex> lock(peers_mutex_);

    fnc(peers_);
  }

  void with_peers_do(std::function<void(std::vector<client_shared_ptr_type>,
                                        std::map<uint64_t, NodeDetails> &)>
                         fnc) {
    std::lock_guard<fetch::mutex::Mutex> lock(peers_mutex_);

    fnc(peers_, server_details_);
  }

  void with_server_details_do(
      std::function<void(std::map<uint64_t, NodeDetails> const &)> fnc) {
    peers_mutex_.lock();
    fnc(server_details_);
    peers_mutex_.unlock();
  }

  void with_node_details(std::function<void(NodeDetails &)> fnc) {
    details_.with_details(fnc);
  }

 private:
  void SendConnectivityDetailsToChainKeepers(
      NodeDetails const &server_details) {
    for (auto const &e2 : server_details.entry_points) {
      fetch::logger.Debug("Testing ", e2.host, ":", e2.port);

      if (e2.configuration & EntryPoint::NODE_CHAIN_KEEPER) {
        chain_keeper_mutex_.lock();
        fetch::logger.Debug(" - Group count = ", chain_keepers_.size());
        for (std::size_t k = 0; k < chain_keepers_.size(); ++k) {
          auto sd = chain_keepers_details_[k];
          fetch::logger.Debug(" - Connect ", e2.host, ":", e2.port, " >> ",
                              sd.host, ":", sd.port, "?");

          if (sd.group == e2.group) {
            std::cout << "       YES!" << std::endl;
            auto sc = chain_keepers_[k];
            sc->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::LISTEN_TO,
                     e2);
          }
        }

        chain_keeper_mutex_.unlock();
      }
    }
  }

  uint64_t protocol_;
  network::ThreadManager *thread_manager_;

  SharedNodeDetails &details_;

  std::map<uint64_t, NodeDetails> client_details_;
  mutable fetch::mutex::Mutex client_details_mutex_;

  std::vector<NodeDetails> peers_with_few_followers_;
  std::unordered_set<std::string> already_seen_;
  mutable fetch::mutex::Mutex suggestion_mutex_;

  std::function<std::string(uint64_t)> request_ip_;

  std::map<uint64_t, NodeDetails> server_details_;
  std::vector<client_shared_ptr_type> peers_;
  mutable fetch::mutex::Mutex peers_mutex_;

  std::vector<client_shared_ptr_type> chain_keepers_;
  std::vector<EntryPoint> chain_keepers_details_;
  mutable fetch::mutex::Mutex chain_keeper_mutex_;

  std::atomic<uint32_t> grouping_parameter_;
};
}
}
#endif
