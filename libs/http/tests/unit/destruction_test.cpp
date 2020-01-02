//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "http/json_client.hpp"
#include "http/json_response.hpp"
#include "http/server.hpp"
#include "json/document.hpp"
#include "network/management/network_manager.hpp"

#include "gtest/gtest.h"

#include <chrono>
#include <iostream>
#include <set>

using namespace fetch::http;
using namespace fetch::network;
using Variant = fetch::variant::Variant;

struct TestModule : HTTPModule
{
  TestModule()
  {
    Get("/test", "Test page",
        [](fetch::http::ViewParameters const & /*params*/,
           fetch::http::HTTPRequest const & /*request*/) {
          return fetch::http::CreateJsonResponse("{}", fetch::http::Status::SUCCESS_OK);
        });
  }
};

using SharedJsonClient = std::shared_ptr<JsonClient>;

std::vector<SharedJsonClient> SimpleTest()
{
  std::vector<SharedJsonClient> ret;
  NetworkManager                network_manager{"Test", 2};

  network_manager.Start();

  // HTTP Server for the agent to interact with the system
  HTTPServer http(network_manager);
  TestModule http_module;
  http.AddModule(http_module);
  http.Start(8000);

  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  SharedJsonClient client =
      std::make_shared<JsonClient>(JsonClient::CreateFromUrl("http://127.0.0.1:8000"));
  ret.push_back(client);
  Variant result;
  client->Post("/test", result);
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  http.Stop();

  network_manager.Stop();

  // Note that we return the shared pointers to keep client side
  // connections alive
  return ret;
}

TEST(HTTPTest, CheckDestruction)
{
  auto clients1 = SimpleTest();
  auto clients2 = SimpleTest();
  auto clients3 = SimpleTest();
  auto clients4 = SimpleTest();
}
