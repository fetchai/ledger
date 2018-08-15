#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "commandline/parameter_parser.hpp"
#include "network/service/server.hpp"
#include "network/tcp/tcp_server.hpp"
#include "protocols.hpp"

#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"
#include "http/server.hpp"

#include <set>

class FetchSwarmService : public fetch::protocols::SwarmProtocol
{
public:
  FetchSwarmService(uint16_t port, uint16_t http_port, std::string const &pk,
                    fetch::network::NetworkManager *tm)
    : fetch::protocols::SwarmProtocol(tm, fetch::protocols::FetchProtocols::SWARM, details_)
    , network_manager_(tm)
    , service_(port, network_manager_)
    , http_server_(http_port, network_manager_)
  {
    using namespace fetch::protocols;

    std::cout << "Listening for peers on " << (port) << ", clients on " << (http_port) << std::endl;

    details_.with_details([=](NodeDetails &details) {
      details.public_key        = pk;
      details.default_port      = port;
      details.default_http_port = http_port;
    });

    EntryPoint e;
    // At this point we don't know what the IP is, but localhost is one entry
    // point
    e.host  = "127.0.0.1";
    e.group = 0;

    e.port          = details_.default_port();
    e.http_port     = details_.default_http_port();
    e.configuration = EntryPoint::NODE_SWARM;
    details_.AddEntryPoint(e);

    running_ = false;

    start_event_ = network_manager_->OnAfterStart([this]() {
      running_ = true;
      network_manager_->io_service().post([this]() { this->UpdateNodeChainKeeperDetails(); });
    });

    stop_event_ = network_manager_->OnBeforeStop([this]() { running_ = false; });

    service_.Add(fetch::protocols::FetchProtocols::SWARM, this);

    // Setting callback to resolve IP
    this->SetClientIPCallback(
        [this](uint64_t const &n) -> std::string { return service_.GetAddress(n); });

    // Creating a http server based on the swarm protocol
    http_server_.AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    http_server_.AddMiddleware(fetch::http::middleware::ColorLog);
    http_server_.AddModule(*this);
  }

  ~FetchSwarmService() {}
  /*
   *  Connectivity maintenance
   *  ═══════════════════════════════════════════
   *  The swarm node continuously updates the
   *  connectivity to other nodes and ensure that
   *  shards are connected to peers. This is done
   *  through following event loop:
   * ┌─────────────────────────────────────────┐
   * │        Update Node ChainKeeper Details  │◀─┐
   * └────────────────────┬────────────────────┘  │
   *                      │                       │
   * ┌────────────────────▼────────────────────┐  │
   * │           Update Peer Details           │  │
   * └────────────────────┬────────────────────┘  │
   *                      │                       │
   * ┌────────────────────▼────────────────────┐  │
   * │               Track peers               │  │
   * └────────────────────┬────────────────────┘  │
   *                      │                       │
   * ┌────────────────────▼────────────────────┐  │
   * │        Update shard connectivity        │  │
   * └────────────────────┬────────────────────┘  │
   *                      │                       │
   * ┌────────────────────▼────────────────────┐  │
   * │           Sync Chain & TX headers       │  │
   * └────────────────────┬────────────────────┘  │
   *                      │                       │
   * ┌────────────────────▼────────────────────┐  │
   * │                   Mine                  │──┘
   * └─────────────────────────────────────────┘
   *

   */
  void UpdateNodeChainKeeperDetails()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    //    fetch::logger.PrintTimings();
    //    fetch::logger.PrintMutexTimings();

    using namespace fetch::protocols;
    std::vector<EntryPoint> entries;

    // Updating shard list
    this->with_shards_do(
        [&entries](std::vector<client_shared_ptr_type> const &sh, std::vector<EntryPoint> &det) {
          std::size_t i = 0;

          for (auto &s : sh)
          {
            auto p = s->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::GROUP_NUMBER);
            if (!p.Wait(2300))
            {
              fetch::logger.Error("ChainKeeper timed out!");
              continue;
            }

            det[i].group = p.As<fetch::group_type>();
            entries.push_back(det[i]);
            ++i;
          }
        });

    // Updating node list
    this->with_node_details([&entries](NodeDetails &details) {
      for (auto &e : details.entry_points)
      {
        for (auto &ref : entries)
        {
          if ((ref.host == e.host) && (e.port == ref.port))
          {
            e.group = ref.group;
            break;
          }
        }
      }
    });

    if (running_)
    {
      network_manager_->io_service().post([this]() { this->UpdatePeerDetails(); });
    }
  }

  void UpdatePeerDetails()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    using namespace fetch::protocols;
    fetch::logger.Highlight("Starting Update Connectivity Loop");

    NodeDetails details;
    this->with_node_details([&details](NodeDetails const &d) { details = d; });

    // Updating outgoing details

    fetch::logger.Highlight("Updating outgoing");
    std::map<fetch::byte_array::ByteArray, NodeDetails> all_details;

    this->with_peers_do([&all_details, details](std::vector<client_shared_ptr_type> const &peers,
                                                std::map<uint64_t, NodeDetails> &peer_details) {
      LOG_STACK_TRACE_POINT;
      for (auto &c : peers)
      {
        auto p = c->Call(FetchProtocols::SWARM, SwarmRPC::HELLO, details);

        if (!p.Wait(2000))
        {
          fetch::logger.Error(
              "Peer connectivity failed! TODO: Trim connections and inform "
              "shards");
          TODO(
              "Peer connectivity failed! TODO: Trim connections and inform "
              "shards");
          continue;
        }

        auto  ref                   = p.As<NodeDetails>();
        auto &d                     = peer_details[c->handle()];
        all_details[ref.public_key] = ref;

        for (auto &e : ref.entry_points)
        {
          fetch::logger.Debug("  - ", e.host, ":", e.port, ", shard ", e.group);
        }

        // TODO: Remove true
        if (true || (d != ref))
        {
          d = ref;
        }
      }
    });

    // Fetching all incoming details

    fetch::logger.Highlight("Updating incoming");
    this->with_client_details_do(
        [&all_details](std::map<uint64_t, NodeDetails> const &node_details) {
          LOG_STACK_TRACE_POINT;
          for (auto const &d : node_details)
          {
            fetch::logger.Debug(" - Entries for ", d.second.public_key);
            for (auto &e : d.second.entry_points)
            {

              fetch::logger.Debug("   > ", e.host, ":", e.port, ", shard ", e.group);
            }

            all_details[d.second.public_key] = d.second;
          }
        });

    all_details[details.public_key] = details;
    this->with_suggestions_do([&all_details](std::vector<NodeDetails> &list) {
      LOG_STACK_TRACE_POINT;
      for (std::size_t i = 0; i < list.size(); ++i)
      {

        auto &details = list[i];

        if (all_details.find(details.public_key) != all_details.end())
        {
          if (details != all_details[details.public_key])
          {
            fetch::logger.Highlight("Updating suggestions info");
            list[i] = all_details[details.public_key];
            TODO("Proapgate change");
            // TODO: Propagate change?
          }
        }
      }
    });

    // Next we track peers
    if (running_)
    {
      network_manager_->io_service().post([this]() { this->TrackPeers(); });
    }
  }

  void TrackPeers()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    using namespace fetch::protocols;

    std::set<fetch::byte_array::ByteArray> public_keys;
    public_keys.insert(this->details_.details().public_key);

    // Finding keys to those we are connected to
    this->with_server_details_do([&](std::map<uint64_t, NodeDetails> const &details) {
      LOG_STACK_TRACE_POINT;
      for (auto const &d : details)
      {
        public_keys.insert(d.second.public_key);
      }
    });

    /*
    this->with_client_details_do([&](std::map< uint64_t, NodeDetails > const &
    details) { for(auto const &d: details)
        {
          public_keys.insert( d.second.public_key );
        }
      });
    */

    // Finding hosts we are not connected to
    std::vector<EntryPoint> swarm_entries;
    this->with_suggestions_do([=, &swarm_entries](std::vector<NodeDetails> const &details) {
      LOG_STACK_TRACE_POINT;
      for (auto const &d : details)
      {
        if (public_keys.find(d.public_key) == public_keys.end())
        {
          for (auto const &e : d.entry_points)
          {
            if (e.configuration & EntryPoint::NODE_SWARM)
            {
              swarm_entries.push_back(e);
            }
          }
        }
      }
    });

    std::random_shuffle(swarm_entries.begin(), swarm_entries.end());
    std::cout << "I wish to connect to: " << std::endl;
    std::size_t i = public_keys.size();

    std::size_t desired_connectivity_ = 5;
    for (auto &e : swarm_entries)
    {
      std::cout << " - " << e.host << ":" << e.port << std::endl;
      this->Bootstrap(e.host, e.port);

      ++i;
      if (i > desired_connectivity_)
      {
        break;
      }
    }

    if (running_)
    {
      network_manager_->io_service().post([this]() { this->UpdateChainKeeperConnectivity(); });
    }
  }

  void UpdateChainKeeperConnectivity()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    using namespace fetch::protocols;

    // Getting the list of shards
    std::vector<EntryPoint> shard_entries;
    this->with_suggestions_do([=, &shard_entries](std::vector<NodeDetails> const &details) {
      LOG_STACK_TRACE_POINT;
      for (auto const &d : details)
      {
        for (auto const &e : d.entry_points)
        {
          if (e.configuration & EntryPoint::NODE_CHAIN_KEEPER)
          {
            shard_entries.push_back(e);
          }
        }
      }
    });

    fetch::logger.Highlight("Updating shards!");
    for (auto &s : shard_entries)
    {
      fetch::logger.Debug(" - ", s.host, ":", s.port, ", shard ", s.group);
    }

    std::random_shuffle(shard_entries.begin(), shard_entries.end());

    // Getting the shard client instances
    std::vector<client_shared_ptr_type> shards;
    std::vector<EntryPoint>             details;

    this->with_shards_do([&shards, &details](std::vector<client_shared_ptr_type> const &sh,
                                             std::vector<EntryPoint> &                  det) {
      LOG_STACK_TRACE_POINT;
      std::size_t i = 0;

      for (auto &s : sh)
      {
        shards.push_back(s);
        details.push_back(det[i]);
        ++i;
      }
    });

    std::cout << "Updating shards: " << std::endl;
    for (std::size_t i = 0; i < shards.size(); ++i)
    {
      auto client = shards[i];
      /*
      auto p2 = client->Call(FetchProtocols::CHAIN_KEEPER,
      ChainKeeperRPC::CHAIN_KEEPER_NUMBER);

      if(! p2.Wait(2300) )
      {
        continue;
      }

      uint32_t shard =  uint32_t( p2  );
      details[i].group = shard;
      */
      std::cout << "  - " << i << " : " << details[i].host << " " << details[i].port << " "
                << details[i].group << std::endl;
      // TODO: set shard detail

      client->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::LISTEN_TO, shard_entries);
    }

    // Updating shard details
    this->with_shards_do(
        [&details](std::vector<client_shared_ptr_type> const &sh, std::vector<EntryPoint> &det) {
          LOG_STACK_TRACE_POINT;
          for (std::size_t i = 0; i < details.size(); ++i)
          {
            if (i < det.size())
            {
              det[i] = details[i];
            }
          }
        });

    if (running_)
    {
      network_manager_->Post([this]() { this->SyncChain(); });
    }
  }

  void SyncChain()
  {
    LOG_STACK_TRACE_POINT;
    // Getting transactions
    using namespace fetch::protocols;
    using block_type = typename ChainController::block_type;

    std::vector<block_type>              blocks;
    std::vector<fetch::service::Promise> promises;
    this->with_peers_do([&promises](std::vector<client_shared_ptr_type> clients) {
      for (auto &c : clients)
      {
        promises.push_back(c->Call(FetchProtocols::SWARM, ChainCommands::GET_BLOCKS));
      }
    });

    std::vector<block_type> newblocks;
    newblocks.reserve(1000);

    for (auto &p : promises)
    {
      p.As(newblocks);
      this->AddBulkBlocks(newblocks);
    }
    fetch::logger.Highlight(
        " >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> "
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
        newblocks.size());

    // Getting transaction summaries
    promises.clear();
    this->with_shards_do([&promises](std::vector<client_shared_ptr_type> const &clients) {
      LOG_STACK_TRACE_POINT;
      for (auto const &c : clients)
      {
        promises.push_back(c->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::GET_SUMMARIES));
      }
    });
    std::vector<fetch::chain::TransactionSummary> summaries;

    for (auto &p : promises)
    {
      p.As(summaries);
      this->AddBulkSummaries(summaries);
    }

    if (running_)
    {
      network_manager_->Post([this]() { this->Mine(); });
    }
  }

  void Mine()
  {
    LOG_STACK_TRACE_POINT;
    bool        adding = true;
    std::size_t i      = 0;

    while (adding)
    {
      adding     = false;
      auto block = this->GetNextBlock();
      if (block.body().transactions.size() > 0)
      {
        this->PushBlock(block);
        adding = true;
      }
      adding &= (i < 200);
      ++i;
    }

    if (running_)
    {
      network_manager_->Post([this]() { this->UpdateNodeChainKeeperDetails(); });
    }
  }

private:
  fetch::network::NetworkManager *                         network_manager_;
  fetch::service::ServiceServer<fetch::network::TCPServer> service_;
  fetch::http::HTTPServer                                  http_server_;

  //  fetch::protocols::SwarmProtocol *swarm_ = nullptr;
  fetch::protocols::SharedNodeDetails details_;

  typename fetch::network::NetworkManager::event_handle_type start_event_;
  typename fetch::network::NetworkManager::event_handle_type stop_event_;
  std::atomic<bool>                                          running_;
};
