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
#include "http/http_client.hpp"
#include "http/https_client.hpp"
#include "http/json_client.hpp"
#include "http/request.hpp"
#include "http/response.hpp"

#include <cerrno>
#include <cstdlib>
#include <ostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

}  // namespace

JsonClient::~JsonClient()                      = default;
JsonClient::JsonClient(JsonClient &&) noexcept = default;

/**
 * Create the JSON client from a specified URL
 *
 * @param url The URL to use for the client
 * @return The constructed JSON Client
 */
JsonClient JsonClient::CreateFromUrl(std::string const &url)
{
  // define the URL pattern that we are matching against
  std::regex const url_pattern{R"(^(https?)://([\w\.-]+)(?::(\d+))?$)"};

  // perform the match
  std::smatch match{};
  bool const  matched = std::regex_match(url, match, url_pattern);
  bool const  success = matched && (match.size() == 4);

  if (!success)
  {
    throw std::runtime_error("Failed to match against URL");
  }

  // extract the matches
  auto const &scheme = match[1];
  auto const &host   = match[2];
  auto const &port   = match[3];

  ConnectionMode mode{ConnectionMode::HTTP};
  if (scheme == "https")
  {
    mode = ConnectionMode::HTTPS;
  }

  if (port.matched)
  {
    // convert the port to an integer (from a string)
    char const *port_str   = port.first.base();
    auto const  port_value = std::strtol(port_str, nullptr, 10);
    if (errno == ERANGE)
    {
      errno = 0;
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to convert port_str=", port_str, " to integer");

      throw std::domain_error(std::string("Failed to convert port_str=") + port_str +
                              " to integer");
    }

    return JsonClient{mode, host, static_cast<uint16_t>(port_value)};
  }
  else
  {
    return JsonClient{mode, host};
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
{}

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
  success = client_->Request(http_request, http_response);

  // attempt to parse the body
  json::JSONDocument doc(http_response.body());
  response = doc.root();

  return success;
}

/**
 * Access to the underlying client
 *
 * @return The reference to the underlying client
 */
HttpClientInterface const &JsonClient::underlying_client() const
{
  return *client_;
}

}  // namespace http
}  // namespace fetch
