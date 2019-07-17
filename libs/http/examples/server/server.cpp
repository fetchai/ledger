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

#include "http/server.hpp"
#include "http/json_response.hpp"
#include "http/validators.hpp"
#include "core/json/document.hpp"

#include <fstream>
#include <iostream>

using namespace fetch::http;
using namespace fetch::json;

struct ExampleModule: HTTPModule
{

  ExampleModule() /*:HTTPModule("ExampleModule", INTERFACE)*/
  {
    Get(
      "/pages",
      "Gets the pages", 
      [](fetch::http::ViewParameters const &/*params*/, fetch::http::HTTPRequest const &/*request*/) {
      return fetch::http::CreateJsonResponse("{}", fetch::http::Status::CLIENT_ERROR_BAD_REQUEST);
    });
    
    Get("/pages/(id=\\d+)/", 
        "Get a specific page",
        {
          {"id", "The page id.", validators::StringValue() }
        },
    [](fetch::http::ViewParameters const &/*params*/, fetch::http::HTTPRequest const &/*request*/) {
      return fetch::http::CreateJsonResponse("{}", fetch::http::Status::CLIENT_ERROR_BAD_REQUEST);
    });
  }
};

int main()
{

  fetch::network::NetworkManager tm{"NetMgr", 1};

  ExampleModule module;
  HTTPServer server(tm);
  server.Start(8080);

  server.AddModule(module);
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
