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

#include "http/json_client.hpp"
#include "http/json_response.hpp"
#include "http/server.hpp"
#include "json/document.hpp"
#include "network/management/network_manager.hpp"

#include "gtest/gtest.h"

#include <chrono>
#include <iostream>
#include <set>
#include <string>

using namespace fetch::http;
using namespace fetch::network;
using Variant = fetch::variant::Variant;

struct TestModule : HTTPModule
{
  TestModule()
  {
    static int counter = 0;
    Get("/test", "Test page",
        [](fetch::http::ViewParameters const & /*params*/,
           fetch::http::HTTPRequest const & /*request*/) {
          return fetch::http::CreateJsonResponse("{\"result\" : " + std::to_string(counter++) + "}",
                                                 fetch::http::Status::SUCCESS_OK);
        });
  }
};

using SharedJsonClient = std::shared_ptr<JsonClient>;

void SimpleTest()
{
  static uint16_t test_port = 8000;

  std::vector<SharedJsonClient> ret;
  {
    NetworkManager network_manager{"Test", 2};

    network_manager.Start();

    // HTTP Server for the agent to interact with the system
    HTTPServer http(network_manager);
    TestModule http_module;
    http.AddModule(http_module);
    http.Start(test_port);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    if (test_port == 8000)
    {
      for (std::size_t i = 0; i < 1; ++i)
      {
        SharedJsonClient client = std::make_shared<JsonClient>(
            JsonClient::CreateFromUrl("http://127.0.0.1:" + std::to_string(test_port)));
        ret.push_back(client);
        Variant result;
        client->Get("/test", result);
      }
    }

    http.Stop();

    network_manager.Stop();

    // Note that the client can outlive the server at the end of this scope
  }
  test_port++;
}

TEST(HTTPTest, CheckDestruction)
{
  SimpleTest();
  SimpleTest();
}
