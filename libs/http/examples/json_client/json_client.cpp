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
#include "core/commandline/params.hpp"

#include <iostream>

using fetch::http::JsonClient;

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  fetch::commandline::Params parser;

  std::string host;
  uint16_t    port = 0;
  std::string method;
  std::string endpoint;
  bool        ssl = false;

  parser.add(host, "host", "The hostname or IP to connect to", std::string{"api.ipify.org"});
  parser.add(port, "port", "The port number to connect to", uint16_t{0});
  parser.add(method, "method", "The http method to be used", std::string{"GET"});
  parser.add(endpoint, "endpoint", "The endpoint to be requested", std::string{"/"});
  parser.add(ssl, "ssl", "The type of the connection being requested", false);

  parser.Parse(argc, argv);

  if (port == 0)
  {
    port = (ssl) ? 443u : 80u;
  }

  // create the client
  JsonClient client(
    (ssl) ? JsonClient::ConnectionMode::HTTPS : JsonClient::ConnectionMode::HTTP,
    host,
    port
  );

  fetch::variant::Variant response;
  if (client.Get("/?format=json", response))
  {
    std::cout << "Response\n\n" << response << std::endl;

    exit_code = EXIT_SUCCESS;
  }
  else
  {
    std::cout << "ERROR: Unable to make query" << std::endl;
  }

  return exit_code;
}
