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

#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

#include <iostream>

using namespace fetch::service;
using namespace fetch::byte_array;

using fetch::muddle::NetworkId;

#include "network/test-helpers/muddle_test_definitions.hpp"

class MuddleTestClient
{
public:
  static std::shared_ptr<MuddleTestClient> CreateTestClient(const std::string &host, uint16_t port)
  {
    return CreateTestClient(Uri(std::string("tcp://") + host + ":" + std::to_string(port)));
  }
  static std::shared_ptr<MuddleTestClient> CreateTestClient(const Uri &uri)
  {
    auto tc = std::make_shared<MuddleTestClient>();
    tc->tm.Start();

    tc->muddle = Muddle::CreateMuddle(NetworkId{"Test"}, tc->tm);
    tc->muddle->Start({});

    tc->client = std::make_shared<Client>("Client", tc->muddle->AsEndpoint(), Address(),
                                          SERVICE_TEST, CHANNEL_RPC);
    tc->muddle->AddPeer(uri);

    int counter = 20;
    while (1)
    {
      if (!counter--)
      {
        tc.reset();
        return tc;
      }
      if (tc->muddle->UriToDirectAddress(uri, tc->address))
      {
        return tc;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return tc;
  }

  bool is_alive()
  {
    return true;
  }

  void Stop()
  {
    muddle->Stop();
    tm.Stop();
  }

  void Start()
  {
    tm.Start();
    muddle->Start({});
  }

  template <typename... Args>
  Promise Call(fetch::service::protocol_handler_type const &protocol,
               fetch::service::function_handler_type const &function, Args &&... args)
  {
    return client->CallSpecificAddress(address, protocol, function, std::forward<Args>(args)...);
  }

  ClientPtr                      client;
  Address                        address;
  MuddlePtr                      muddle;
  fetch::network::NetworkManager tm{"NetMgr", 1};

  MuddleTestClient()
  {}
};

using TClientPtr = std::shared_ptr<MuddleTestClient>;
