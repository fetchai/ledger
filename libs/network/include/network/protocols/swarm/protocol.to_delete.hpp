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

#include "http/module.hpp"
#include "json/document.hpp"
#include "protocols/chain_keeper/commands.hpp"
#include "protocols/fetch_protocols.hpp"
#include "protocols/swarm/commands.hpp"
#include "protocols/swarm/controller.hpp"
#include "service/function.hpp"

namespace fetch {
namespace protocols {
class SwarmProtocol : public SwarmController,
                      public fetch::service::Protocol,
                      public fetch::http::HTTPModule
{
public:
  SwarmProtocol(network::NetworkManager *network_manager, uint64_t const &protocol,
                SharedNodeDetails &details)
    : SwarmController(protocol, network_manager, details), fetch::service::Protocol()
  {
    using namespace fetch::service;

    // RPC Protocol
    SwarmController *controller = (SwarmController *)this;

    Protocol::Expose(SwarmRPC::PING, controller, &SwarmController::Ping);
    Protocol::ExposeWithClientArg(SwarmRPC::HELLO, controller, &SwarmController::Hello);
    Protocol::Expose(SwarmRPC::SUGGEST_PEERS, controller, &SwarmController::SuggestPeers);
    Protocol::Expose(SwarmRPC::REQUEST_PEER_CONNECTIONS, controller,
                     &SwarmController::RequestPeerConnections);
    Protocol::ExposeWithClientArg(SwarmRPC::WHATS_MY_IP, controller, &SwarmController::GetAddress);

    // Using the event feed that
    Protocol::RegisterFeed(SwarmFeed::FEED_REQUEST_CONNECTIONS, this);
    Protocol::RegisterFeed(SwarmFeed::FEED_ENOUGH_CONNECTIONS, this);
    Protocol::RegisterFeed(SwarmFeed::FEED_ANNOUNCE_NEW_COMER, this);

    ChainController *ccontroller = (ChainController *)this;
    Protocol::Expose(ChainCommands::PUSH_BLOCK, ccontroller, &ChainController::PushBlock);
    Protocol::Expose(ChainCommands::GET_BLOCKS, ccontroller, &ChainController::GetLatestBlocks);
    Protocol::Expose(ChainCommands::GET_NEXT_BLOCK, ccontroller, &ChainController::GetNextBlock);

    auto all_details = [this](fetch::http::ViewParameters const &params,
                              fetch::http::HTTPRequest const &   req) {
      LOG_STACK_TRACE_POINT;
      std::stringstream response;

      response << "{\"blocks\": [";
      this->with_blocks_do(
          [&response](ChainManager::shared_block_type block, ChainManager::chain_map_type &chain) {
            std::size_t i = 0;
            while ((i < 10) && (block))
            {
              if (i != 0) response << ", ";
              response << "{";
              response << "\"block_hash\": \"" << byte_array::ToBase64(block->header()) << "\",";
              auto prev = block->body().previous_hash;

              response << "\"previous_hash\": \"" << byte_array::ToBase64(prev) << "\",";

              // byte_array::ToBase64( block->body().transaction_hash )
              response << "\"count\": " << block->body().transactions.size() << ", ";
              response << "\"transactions\": [";
              bool bfit = true;
              for (auto tx : block->body().transactions)
              {
                if (!bfit)
                {
                  response << ", ";
                }
                response << "{\"hash\":\"" << byte_array::ToBase64(tx.transaction_hash) << "\",";
                bool cit = true;

                response << "\"groups\": [";
                for (auto &g : tx.groups)
                {
                  if (!cit) response << ", ";
                  response << g;
                  cit = false;
                }

                response << "]";
                response << "}";

                bfit = false;
              }

              response << "]"
                       << ",";
              response << "\"block_number\": " << block->block_number() << ",";
              response << "\"total_work\": " << block->total_weight();
              response << "}";
              block = block->previous();
              ++i;
            }
          });

      response << "], ";
      std::size_t shard_count = 0;

      response << "\"shards\": [";

      this->with_shard_details_do(
          [&shard_count, &response](std::vector<EntryPoint> const &detail_list) {
            bool first = true;

            for (auto &d : detail_list)
            {
              if (!first) response << ",";
              response << "{ \"host\": \"" << d.host << "\",";
              response << " \"port\": " << d.port << ",";
              response << " \"shard\": " << d.group << ",";
              response << " \"http_port\": " << d.http_port << "}";
              first = false;
              ++shard_count;
            }
          });
      response << "], ";

      response << "\"outgoing\": [";

      this->with_server_details_do([&response](std::map<uint64_t, NodeDetails> const &peers) {
        bool first = true;
        for (auto &ppair : peers)
        {
          auto &p = ppair.second;
          if (!first) response << ", \n";
          response << "{\n";
          response << "\"public_key\": \"" + p.public_key + "\",";
          response << "\"entry_points\": [";
          bool sfirst = true;

          for (auto &e : p.entry_points)
          {
            if (!sfirst) response << ",\n";
            response << "{";
            response << "\"shard\": " << e.group << ",";
            response << "\"host\": \"" << e.host << "\",";
            response << "\"port\": " << e.port << ",";
            response << "\"http_port\": " << e.http_port << ",";
            response << "\"configuration\": " << e.configuration << "}";
            sfirst = false;
          }

          response << "]";
          response << "}";
          first = false;
        }
      });

      response << "], ";
      response << "\"incoming\": [";

      this->with_client_details_do([&response](std::map<uint64_t, NodeDetails> const &peers) {
        bool first = true;
        for (auto &ppair : peers)
        {
          auto &p = ppair.second;
          if (!first) response << ", \n";
          response << "{\n";
          response << "\"public_key\": \"" + p.public_key + "\",";
          response << "\"entry_points\": [";
          bool sfirst = true;

          for (auto &e : p.entry_points)
          {
            if (!sfirst) response << ",\n";
            response << "{";
            response << "\"shard\": " << e.group << ",";
            response << "\"host\": \"" << e.host << "\",";
            response << "\"port\": " << e.port << ",";
            response << "\"http_port\": " << e.http_port << ",";
            response << "\"configuration\": " << e.configuration << "}";
            sfirst = false;
          }

          response << "]";
          response << "}";
          first = false;
        }
      });

      response << "], ";
      response << "\"suggestions\": [";

      this->with_suggestions_do([&response](std::vector<NodeDetails> const &peers) {
        bool first = true;
        for (auto &p : peers)
        {
          if (!first) response << ", \n";
          response << "{\n";
          response << "\"public_key\": \"" + p.public_key + "\",";
          response << "\"entry_points\": [";
          bool sfirst = true;

          for (auto &e : p.entry_points)
          {
            if (!sfirst) response << ",\n";
            response << "{";
            response << "\"shard\": " << e.group << ",";
            response << "\"host\": \"" << e.host << "\",";
            response << "\"port\": " << e.port << ",";
            response << "\"http_port\": " << e.http_port << ",";
            response << "\"configuration\": " << e.configuration << "}";
            sfirst = false;
          }

          response << "]";
          response << "}";
          first = false;
        }
      });

      response << "], ";
      uint16_t              port = 0, http_port = 0;
      byte_array::ByteArray host;

      this->with_node_details([&response, &port, &http_port, &host](NodeDetails const &details) {
        response << "\"name\": \"" << details.public_key << "\",";
        response << "\"entry_points\": [";
        bool first = true;

        for (auto &e : details.entry_points)
        {
          if (!first) response << ", ";
          response << "{";
          response << "\"shard\": " << e.group << ",";
          response << "\"host\": \"" << e.host << "\",";
          response << "\"port\": " << e.port << ",";
          response << "\"http_port\": " << e.http_port << ",";
          response << "\"configuration\": " << e.configuration << "}";
          if (e.configuration & EntryPoint::NODE_SWARM)
          {
            port      = e.port;
            http_port = e.http_port;
            host      = e.host;
          }

          first = false;
        }

        response << "]";
      });
      if ((host.size() > 0)) response << ",\"host\": \"" << host << "\"";

      if (port != 0) response << ",\"port\": " << port;
      if (http_port != 0) response << ",\"http_port\": " << http_port;
      response << "}";
      return fetch::http::HTTPResponse(response.str());
    };
    HTTPModule::Get("/all-details", all_details);

    auto list_blocks = [this](fetch::http::ViewParameters const &params,
                              fetch::http::HTTPRequest const &   req) {
      LOG_STACK_TRACE_POINT;
      std::stringstream response;

      response << "{\"blocks\": [";
      this->with_blocks_do(
          [&response](ChainManager::shared_block_type block, ChainManager::chain_map_type &chain) {
            std::size_t i = 0;
            while ((i < 10) && (block))
            {
              if (i != 0) response << ", ";
              response << "{";
              response << "\"block_hash\": \"" << byte_array::ToBase64(block->header()) << "\",";
              auto prev = block->body().previous_hash;

              response << "\"previous_hash\": \"" << byte_array::ToBase64(prev) << "\",";

              // byte_array::ToBase64( block->body().transaction_hash )
              response << "\"count\": " << block->body().transactions.size() << ", ";
              response << "\"transactions\": [";
              bool bfit = true;
              for (auto tx : block->body().transactions)
              {
                if (!bfit)
                {
                  response << ", ";
                }
                response << "\"" << byte_array::ToBase64(tx.transaction_hash) << "\"";
                bfit = false;
              }

              response << "]"
                       << ",";
              response << "\"block_number\": " << block->block_number() << ",";
              response << "\"total_work\": " << block->total_weight();
              response << "}";
              block = block->previous();
              ++i;
            }
          });

      response << "]}";
      return fetch::http::HTTPResponse(response.str());
    };
    HTTPModule::Get("/list/blocks", list_blocks);

    // Web interface
    auto http_bootstrap = [this](fetch::http::ViewParameters const &params,
                                 fetch::http::HTTPRequest const &   req) {
      this->Bootstrap(params["ip"], uint16_t(params["port"].AsInt()));
      return fetch::http::HTTPResponse("{\"status\":\"ok\"}");
    };

    HTTPModule::Get("/bootstrap/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(port=\\d+)",
                    http_bootstrap);
    HTTPModule::Get(
        "/connect-to/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/"
        "(port=\\d+)",
        http_bootstrap);

    auto shard_connect = [this](fetch::http::ViewParameters const &params,
                                fetch::http::HTTPRequest const &   req) {
      std::cout << "Connecting shard from HTTP" << std::endl;

      this->ConnectChainKeeper(params["ip"], uint16_t(params["port"].AsInt()));
      return fetch::http::HTTPResponse("{\"status\":\"ok\"}");
    };
    HTTPModule::Get(
        "/connect-shard/(ip=\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/"
        "(port=\\d+)",
        shard_connect);

    auto list_shards = [this](fetch::http::ViewParameters const &params,
                              fetch::http::HTTPRequest const &   req) {
      std::stringstream response;
      response << "{ \"shards\": [";

      this->with_shard_details_do([&response](std::vector<EntryPoint> const &detail_list) {
        bool first = true;

        for (auto &d : detail_list)
        {
          if (!first) response << ",";
          response << "{ \"host\": \"" << d.host << "\",";
          response << " \"port\": " << d.port << ",";
          response << " \"shard\": " << d.group << ",";
          response << " \"http_port\": " << d.http_port << "}";
          first = false;
        }
      });
      response << "] }";

      return fetch::http::HTTPResponse(response.str());
    };
    HTTPModule::Get("/list/shards", list_shards);

    auto list_outgoing = [this](fetch::http::ViewParameters const &params,
                                fetch::http::HTTPRequest const &   req) {
      std::stringstream response;

      response << "{\"outgoing\": [";

      this->with_server_details_do([&response](std::map<uint64_t, NodeDetails> const &peers) {
        bool first = true;
        for (auto &ppair : peers)
        {
          auto &p = ppair.second;
          if (!first) response << ", \n";
          response << "{\n";
          response << "\"public_key\": \"" + p.public_key + "\",";
          response << "\"entry_points\": [";
          bool sfirst = true;

          for (auto &e : p.entry_points)
          {
            if (!sfirst) response << ",\n";
            response << "{";
            response << "\"shard\": " << e.group << ",";
            response << "\"host\": \"" << e.host << "\",";
            response << "\"port\": " << e.port << ",";
            response << "\"http_port\": " << e.http_port << ",";
            response << "\"configuration\": " << e.configuration << "}";
            sfirst = false;
          }

          response << "]";
          response << "}";
          first = false;
        }
      });

      response << "]}";

      std::cout << response.str() << std::endl;

      return fetch::http::HTTPResponse(response.str());
    };

    HTTPModule::Get("/list/outgoing", list_outgoing);

    auto list_incoming = [this](fetch::http::ViewParameters const &params,
                                fetch::http::HTTPRequest const &   req) {
      std::stringstream response;

      response << "{\"incoming\": [";

      this->with_client_details_do([&response](std::map<uint64_t, NodeDetails> const &peers) {
        bool first = true;
        for (auto &ppair : peers)
        {
          auto &p = ppair.second;
          if (!first) response << ", \n";
          response << "{\n";
          response << "\"public_key\": \"" + p.public_key + "\",";
          response << "\"entry_points\": [";
          bool sfirst = true;

          for (auto &e : p.entry_points)
          {
            if (!sfirst) response << ",\n";
            response << "{";
            response << "\"shard\": " << e.group << ",";
            response << "\"host\": \"" << e.host << "\",";
            response << "\"port\": " << e.port << ",";
            response << "\"http_port\": " << e.http_port << ",";
            response << "\"configuration\": " << e.configuration << "}";
            sfirst = false;
          }

          response << "]";
          response << "}";
          first = false;
        }
      });

      response << "]}";
      std::cout << response.str() << std::endl;

      return fetch::http::HTTPResponse(response.str());
    };

    HTTPModule::Get("/list/incoming", list_incoming);

    auto list_suggestions = [this](fetch::http::ViewParameters const &params,
                                   fetch::http::HTTPRequest const &   req) {
      std::stringstream response;

      response << "{\"suggestions\": [";

      this->with_suggestions_do([&response](std::vector<NodeDetails> const &peers) {
        bool first = true;
        for (auto &p : peers)
        {
          if (!first) response << ", \n";
          response << "{\n";
          response << "\"public_key\": \"" + p.public_key + "\",";
          response << "\"entry_points\": [";
          bool sfirst = true;

          for (auto &e : p.entry_points)
          {
            if (!sfirst) response << ",\n";
            response << "{";
            response << "\"shard\": " << e.group << ",";
            response << "\"host\": \"" << e.host << "\",";
            response << "\"port\": " << e.port << ",";
            response << "\"http_port\": " << e.http_port << ",";
            response << "\"configuration\": " << e.configuration << "}";
            sfirst = false;
          }

          response << "]";
          response << "}";
          first = false;
        }
      });

      response << "]}";
      std::cout << response.str() << std::endl;

      return fetch::http::HTTPResponse(response.str());
    };
    HTTPModule::Get("/list/suggestions", list_suggestions);

    auto node_details = [this](fetch::http::ViewParameters const &params,
                               fetch::http::HTTPRequest const &   req) {
      std::stringstream response;
      response << "{";

      this->with_node_details([&response](NodeDetails const &details) {
        response << "\"name\": \"" << details.public_key << "\",";
        response << "\"entry_points\": [";
        bool first = true;

        for (auto &e : details.entry_points)
        {
          if (!first) response << ", ";
          response << "{";
          response << "\"shard\": " << e.group << ",";
          response << "\"host\": \"" << e.host << "\",";
          response << "\"port\": " << e.port << ",";
          response << "\"http_port\": " << e.http_port << ",";
          response << "\"configuration\": " << e.configuration << "}";

          first = false;
        }

        response << "]";
      });

      response << "}";
      return fetch::http::HTTPResponse(response.str());
    };
    HTTPModule::Get("/node-details", node_details);

    ////// TODO: This part needs to be moved somewhere else in the future
    auto send_transaction = [this](fetch::http::ViewParameters const &params,
                                   fetch::http::HTTPRequest const &   req) {
      json::JSONDocument doc = req.JSON();

      std::cout << "Got : " << req.uri() << std::endl;
      std::cout << "Doc root " << doc.root() << std::endl;
      std::cout << "resources " << doc["resources"] << std::endl;

      this->with_shards_do([req](std::vector<client_shared_ptr_type> shards,
                                 std::vector<EntryPoint> const &     detail_list) {
        std::cout << "Sending tx to " << shards.size() << " shards" << std::endl;
        using transaction_type = fetch::chain::Transaction;
        transaction_type tx;
        tx.set_arguments(req.body());
        for (auto &s : shards)
        {
          s->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::PUSH_TRANSACTION, tx);
        }
      });

      return fetch::http::HTTPResponse("{}");
    };

    HTTPModule::Get("/load-balancer/send-transaction", send_transaction);

    auto increase_shard = [this](fetch::http::ViewParameters const &params,
                                 fetch::http::HTTPRequest const &   req) {
      this->IncreaseGroupingParameter();
      return fetch::http::HTTPResponse("{}");
    };

    HTTPModule::Get("/increase-grouping-parameter", increase_shard);
  }
};
}  // namespace protocols
}  // namespace fetch
