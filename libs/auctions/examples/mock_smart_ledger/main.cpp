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

#include "core/commandline/params.hpp"
#include <iostream>

#include "http/server.hpp"
#include "mock_smart_ledger.hpp"
#include "network/management/network_manager.hpp"

#include <chrono>
#include <thread>

int main(int argc, char **argv)
{

  fetch::commandline::Params parser;

  std::string host;
  uint16_t    port = 0;
  std::string method;
  std::string endpoint;

  parser.add(host, "host", "The hostname or IP to connect to");
  parser.add(port, "port", "The port number to connect to", uint16_t{80});
  parser.add(method, "method", "The http method to be used", std::string{"GET"});
  parser.add(endpoint, "endpoint", "The endpoint to be requested", std::string{"/"});

  parser.Parse(argc, argv);

  // create the client
  fetch::auctions::examples::MockSmartLedger msl{};

  fetch::network::NetworkManager nm{"mock_smart_ledger_network_manager", 8};
  fetch::http::HTTPServer        server(nm);
  server.Start(8080);
  server.AddModule(msl);
  nm.Start();

  std::this_thread::sleep_for(std::chrono::minutes(1));
}
