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

#include "http_client_interface.hpp"
#include "network/fetch_asio.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>

namespace fetch {
namespace http {
class HTTPRequest;
class HTTPResponse;
}  // namespace http
}  // namespace fetch

namespace fetch {
namespace http {

/**
 * Simple blocking HTTP client used for querying information
 */
class HttpClient : public HttpClientInterface
{
public:
  static constexpr char const *LOGGING_NAME = "HTTPClient";
  static constexpr uint16_t    DEFAULT_PORT = 80;

  // Construction / Destruction
  explicit HttpClient(std::string host, uint16_t port = DEFAULT_PORT);
  ~HttpClient() override = default;

  /// @name Accessors
  /// @{
  std::string const &host() const;
  uint16_t           port() const;
  /// @}

  /// @name Http Client Interface
  /// @{
  bool Request(HTTPRequest const &request, HTTPResponse &response) override;
  /// @}

protected:
  using IoService = asio::io_service;
  using Socket    = asio::ip::tcp::socket;

  /// @name HTTP Client Methods
  /// @{
  virtual bool        Connect();
  virtual void        Write(asio::streambuf const &buffer, std::error_code &ec);
  virtual std::size_t ReadUntil(asio::streambuf &buffer, char const *delimiter,
                                std::error_code &ec);
  virtual void        ReadExactly(asio::streambuf &buffer, std::size_t length, std::error_code &ec);
  /// @}

  IoService io_service_;

private:
  std::string host_;
  uint16_t    port_;
  Socket      socket_{io_service_};
};

inline std::string const &HttpClient::host() const
{
  return host_;
}

inline uint16_t HttpClient::port() const
{
  return port_;
}

}  // namespace http
}  // namespace fetch
