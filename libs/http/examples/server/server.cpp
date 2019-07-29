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

#include "core/json/document.hpp"
#include "http/json_response.hpp"
#include "http/middleware/deny_all.hpp"
#include "http/middleware/token_auth.hpp"
#include "http/server.hpp"
#include "http/validators.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

using namespace fetch::http;
using namespace fetch::json;

struct ExampleModule : HTTPModule
{

  ExampleModule() /*:HTTPModule("ExampleModule", INTERFACE)*/
  {
    Get("/pages", "Gets the pages",
        [](fetch::http::ViewParameters const & /*params*/,
           fetch::http::HTTPRequest const & /*request*/) {
          return fetch::http::CreateJsonResponse("{}", fetch::http::Status::SUCCESS_OK);
        });

    Get("/pages/(id=\\d+)", "Get a specific page",
        {{"id", "The page id.", validators::StringValue()}},
        [](fetch::http::HTTPRequest req) {
          if (req.authentication_level() < 900)
          {
            return false;
          }
          return true;
        },
        [](fetch::http::ViewParameters const & /*params*/,
           fetch::http::HTTPRequest const & /*request*/) {
          return fetch::http::CreateJsonResponse(R"({"error": "It's all good!"})",
                                                 fetch::http::Status::SUCCESS_OK);
        });

    Get("/throw", "Throws an exception",
        [](fetch::http::ViewParameters const & /*params*/,
           fetch::http::HTTPRequest const & /*request*/) {
          throw std::runtime_error("some exception!");
          return fetch::http::CreateJsonResponse("{}",
                                                 fetch::http::Status::CLIENT_ERROR_BAD_REQUEST);
        });
  }
};

int main()
{
  fetch::network::NetworkManager tm{"NetMgr", 1};

  ExampleModule module;
  HTTPServer    server(tm);
  server.Start(8080);

  server.AddModule(module);
  // server.AddMiddleware(middleware::DenyAll()); //< Add this line to deny all requests unless
  // authenticated
  server.AddMiddleware(middleware::TokenAuth("hello"));
  server.AddMiddleware([](HTTPRequest &) { std::cout << "Middleware 1" << std::endl; });
  server.AddMiddleware([](HTTPResponse &res, HTTPRequest const &req) {
    std::cout << static_cast<uint16_t>(res.status()) << " " << req.uri() << std::endl;
  });

  tm.Start();

  std::cout << "HTTP server on port 8080" << std::endl;
  std::cout << "Ctrl-C to stop" << std::endl;
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  tm.Stop();

  return 0;
}
