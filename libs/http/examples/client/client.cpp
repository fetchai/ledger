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
#include "http/http_client.hpp"
#include "http/https_client.hpp"
#include "http/request.hpp"
#include "http/response.hpp"

#include <iostream>
#include <memory>

using fetch::http::HttpClientInterface;
using fetch::http::HttpClient;
using fetch::http::HttpsClient;

using ClientPtr = std::unique_ptr<HttpClientInterface>;

int main(int argc, char **argv)
{
  int exit_code = EXIT_FAILURE;

  fetch::commandline::Params parser;

  std::string host;
  uint16_t    port = 0;
  std::string method;
  std::string endpoint;
  bool        ssl = false;

  parser.add(host, "host", "The hostname or IP to connect to");
  parser.add(port, "port", "The port number to connect to", uint16_t{0});
  parser.add(endpoint, "endpoint", "The endpoint to be requested", std::string{"/"});
  parser.add(ssl, "ssl", "Use SSL for the configuration", false);

  parser.Parse(argc, argv);

  // if the default port is selected then choose the correct default
  if (port == 0)
  {
    port = (ssl) ? HttpsClient::DEFAULT_PORT : HttpClient::DEFAULT_PORT;
  }

  std::cout << "Host     : " << host << std::endl;
  std::cout << "Port     : " << port << std::endl;
  std::cout << "Endpoint : GET " << endpoint << std::endl;
  std::cout << "SSL      : " << ssl << std::endl;

  // create the client
  ClientPtr client =
      (ssl) ? std::make_unique<HttpsClient>(host, port) : std::make_unique<HttpClient>(host, port);

  // make the request
  fetch::http::HTTPRequest req;
  req.SetMethod(fetch::http::Method::GET);
  req.SetURI(endpoint);

  fetch::http::HTTPResponse response;
  if (client->Request(req, response))
  {
    std::cout << "Status Code: " << ToString(response.status()) << std::endl;
    std::cout << response.body() << std::endl;

    exit_code = EXIT_SUCCESS;
  }
  else
  {
    std::cout << "Failed to make the request" << std::endl;
  }

  return exit_code;
}
