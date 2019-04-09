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
#include "http/https_client.hpp"
#include "http/http_client.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "core/json/document.hpp"

#include <sstream>
#include <utility>

namespace fetch {
namespace http {
namespace {

  /**
   * Select the connect default port based on the target connection mode
   *
   * @param mode The connection mode
   * @return The default port for that connection type
   */
  uint16_t MapPort(JsonClient::ConnectionMode mode)
  {
    uint16_t port{0};

    switch (mode)
    {
    case JsonClient::ConnectionMode::HTTP:
      port = HttpClient::DEFAULT_PORT;
      break;
    case JsonClient::ConnectionMode::HTTPS:
      port = HttpsClient::DEFAULT_PORT;
      break;
    }

    return port;
  }

}

/**
 * Construct a JsonClient from a mode and host
 *
 * @param mode The connection to be used
 * @param host The hostname or IP address to connect to
 */
JsonClient::JsonClient(ConnectionMode mode, std::string host)
  : JsonClient{mode, std::move(host), MapPort(mode)}
{
}

/**
 * Construct a JsonClient from a mode, host and port
 *
 * @param mode The connection mode to be used
 * @param host The hostname or IP address to connect to
 * @param port The port number to connect on
 */
JsonClient::JsonClient(ConnectionMode mode, std::string host, uint16_t port)
{
  // based on the connection mode choose the corresponding client implementation
  switch (mode)
  {
  case ConnectionMode::HTTP:
    client_ = std::make_unique<HttpClient>(std::move(host), port);
    break;
  case ConnectionMode::HTTPS:
    client_ = std::make_unique<HttpsClient>(std::move(host), port);
    break;
  }

  if (!client_)
  {
    throw std::runtime_error("Unable to create target HTTP(S) client");
  }
}

/**
 * Internal: Make the underlying HTTP request
 *
 * @param method The HTTP method that should be used
 * @param endpoint The endpoint for the request i.e. '/'
 * @param headers The (optional) pointer to a collection of custom headers to be provided
 * @param request The (optional) pointer to the request payload
 * @param response The output response from the server
 * @return true if successful, otherwise false
 */
bool JsonClient::Request(Method method, ConstByteArray const &endpoint, Headers const *headers,
                             Variant const *request, Variant &response)
{
  bool success = false;

  // make the request
  HTTPRequest http_request;
  http_request.SetMethod(method);
  http_request.SetURI(endpoint);

  if (headers)
  {
    for (auto const &element : *headers)
    {
      http_request.AddHeader(element.first, element.second);
    }
  }

  if (request)
  {
    std::ostringstream oss;
    oss << *request;
    http_request.SetBody(oss.str());

    // override the content type
    http_request.AddHeader("Content-Type", "application/json");
  }

  fetch::http::HTTPResponse http_response;
  if (client_->Request(http_request, http_response))
  {
    success = true;

    json::JSONDocument doc(http_response.body());
    response = doc.root();
  }

  return success;
}

}  // namespace http
}  // namespace fetch
