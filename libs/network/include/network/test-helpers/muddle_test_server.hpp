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

#include "network/test-helpers/muddle_test_definitions.hpp"

using namespace fetch::service;
using namespace fetch::byte_array;

class MuddleTestServer
{
public:
  static std::shared_ptr<MuddleTestServer> CreateTestServer(uint16_t port)
  {
    using fetch::muddle::NetworkId;

    auto ts = std::make_shared<MuddleTestServer>();
    ts->tm.Start();

    ts->port   = port;
    ts->muddle = Muddle::CreateMuddle(NetworkId{"Test"}, ts->tm);
    ts->muddle->Start({port});

    ts->server = std::make_shared<Server>(ts->muddle->AsEndpoint(), SERVICE_TEST, CHANNEL_RPC);

    return ts;
  }

  template <class X, class Y>
  void Add(X &x, Y &y)
  {
    server->Add(x, y);
  }

  template <class X, class Y>
  void Add(X x, Y y)
  {
    server->Add(x, y);
  }

  template <class X, class Y>
  void Add(const X &x, Y &y)
  {
    server->Add(x, y);
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
    muddle->Start({port});
  }

  ServerPtr                      server;
  uint16_t                       port;
  MuddlePtr                      muddle;
  fetch::network::NetworkManager tm{"NetMgr", 1};

  MuddleTestServer()
  {}
};

using TServerPtr = std::shared_ptr<MuddleTestServer>;
