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

// This file holds and manages connections to other nodes
// Not for long-term use

#include "core/byte_array/byte_array.hpp"
#include "core/logger.hpp"
#include "helper_functions.hpp"
#include "ledger/chain/transaction.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/service/server.hpp"
#include "network/service/service_client.hpp"
#include "network_classes.hpp"
#include "protocols/fetch_protocols.hpp"
#include "protocols/network_benchmark/commands.hpp"
#include "protocols/network_mine_test/commands.hpp"
#include <iostream>

#include "network/test-helpers/muddle_test_client.hpp"
#include "network/test-helpers/muddle_test_definitions.hpp"
#include "network/test-helpers/muddle_test_server.hpp"

using TServerPtr = std::shared_ptr<MuddleTestServer>;
using TClientPtr = std::shared_ptr<MuddleTestClient>;

#include <set>
#include <utility>

namespace fetch {
namespace network_benchmark {

class NodeDirectory
{
public:
  static constexpr char const *LOGGING_NAME = "NodeDirectory";

  NodeDirectory() = default;

  NodeDirectory(NodeDirectory &rhs)  = delete;
  NodeDirectory(NodeDirectory &&rhs) = delete;
  NodeDirectory operator=(NodeDirectory &rhs) = delete;
  NodeDirectory operator=(NodeDirectory &&rhs) = delete;

  ~NodeDirectory() = default;

  // Only call this during node setup (not thread safe)
  void AddEndpoint(const Endpoint &endpoint)
  {
    LOG_STACK_TRACE_POINT;
    if (serviceClients_.find(endpoint) == serviceClients_.end())
    {
      auto client = MuddleTestClient::CreateTestClient(endpoint.IP(), endpoint.TCPPort());
      serviceClients_[endpoint] = client;
    }
  }

  // push headers to the rest of the network
  template <typename T>
  void PushBlock(T block)
  {
    LOG_STACK_TRACE_POINT;

    for (auto &i : serviceClients_)
    {
      auto client = i.second;

      if (!client->is_alive())
      {
        std::cerr << "Client has died (pushing)!\n\n" << std::endl;
        FETCH_LOG_ERROR(LOGGING_NAME, "Client has died in node direc");
      }

      client->Call(protocols::FetchProtocols::NETWORK_MINE_TEST,
                   protocols::NetworkMineTest::PUSH_NEW_HEADER, block);
    }
  }

  template <typename H, typename T>
  bool GetHeader(H hash, T &block)
  {
    LOG_STACK_TRACE_POINT;

    for (auto &i : serviceClients_)
    {
      auto client = i.second;

      if (!client->is_alive())
      {
        std::cerr << "Client has died (pulling)!\n\n" << std::endl;
        FETCH_LOG_ERROR(LOGGING_NAME, "Client has died in node direc");
      }

      std::pair<bool, T> result = client
                                      ->Call(protocols::FetchProtocols::NETWORK_MINE_TEST,
                                             protocols::NetworkMineTest::PROVIDE_HEADER, hash)
                                      ->template As<std::pair<bool, T>>();

      if (result.first)
      {
        result.second.UpdateDigest();
        block = result.second;
        return true;
      }
    }

    return false;
  }

  // temporarily replicate invite functionality for easier debugging
  void InviteAllForw(block_hash const &blockHash, block_type &block)
  {
    LOG_STACK_TRACE_POINT;

    for (auto &i : serviceClients_)
    {
      auto client = i.second;

      if (!client->is_alive())
      {
        std::cerr << "Client has died (forw)!\n\n" << std::endl;
      }

      bool clientWants = client
                             ->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
                                    protocols::NetworkBenchmark::INVITE_PUSH, blockHash)
                             ->As<bool>();

      if (clientWants)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Client wants forwarded push");
        client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
                     protocols::NetworkBenchmark::PUSH, blockHash, block);
      }
    }
  }

  void InviteAllDirect(block_hash const &blockHash, block_type const &block)
  {
    LOG_STACK_TRACE_POINT;

    for (auto &i : serviceClients_)
    {
      auto client = i.second;

      if (!client->is_alive())
      {
        std::cerr << "Client has died!\n\n" << std::endl;
        exit(1);
      }

      client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
                   protocols::NetworkBenchmark::PUSH_CONFIDENT, blockHash, block);
    }
  }

  void InviteAllBlocking(block_hash const &blockHash, block_type const &block)
  {
    LOG_STACK_TRACE_POINT;

    for (auto &i : serviceClients_)
    {
      auto client = i.second;

      if (!client->is_alive())
      {
        std::cerr << "Client has died!\n\n" << std::endl;
        exit(1);
      }

      auto p1 = client->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
                             protocols::NetworkBenchmark::PUSH_CONFIDENT, blockHash, block);

      FETCH_LOG_PROMISE();
      p1->Wait();
    }
  }

  void ControlSlaves()
  {
    LOG_STACK_TRACE_POINT;

    for (auto &i : serviceClients_)
    {
      auto client = i.second;

      if (!client->is_alive())
      {
        std::cerr << "Client to slave has died!\n\n" << std::endl;
        exit(1);
      }

      while (client
                 ->Call(protocols::FetchProtocols::NETWORK_BENCHMARK,
                        protocols::NetworkBenchmark::SEND_NEXT)
                 ->As<bool>())
      {
      }
    }
  }

  void Reset()
  {
    serviceClients_.clear();
  }

private:
  std::map<Endpoint, TClientPtr> serviceClients_;
};

}  // namespace network_benchmark
}  // namespace fetch
