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

#include "http/http_client.hpp"
#include "network/fetch_asio.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>

namespace fetch {
namespace http {

class HttpsClient : public HttpClient
{
public:
  static constexpr uint16_t DEFAULT_PORT = 443;

  // Construction / Destruction
  explicit HttpsClient(std::string host, uint16_t port = DEFAULT_PORT);
  HttpsClient(HttpsClient const &) = delete;
  HttpsClient(HttpsClient &&)      = delete;
  ~HttpsClient() override          = default;

  // Operators
  HttpsClient &operator=(HttpsClient const &) = delete;
  HttpsClient &operator=(HttpsClient &&) = delete;

protected:
  /// @name HTTP Client Controls
  /// @{
  bool        Connect() override;
  void        Write(asio::streambuf const &buffer, std::error_code &ec) override;
  std::size_t ReadUntil(asio::streambuf &buffer, char const *delimiter,
                        std::error_code &ec) override;
  void ReadExactly(asio::streambuf &buffer, std::size_t length, std::error_code &ec) override;
  /// @}

private:
  using Socket     = asio::ip::tcp::socket;
  using IoService  = asio::io_service;
  using SslContext = asio::ssl::context;
  using Stream     = asio::ssl::stream<Socket>;

  SslContext context_{SslContext::sslv23};
  Stream     socket_{io_service_, context_};
};

}  // namespace http
}  // namespace fetch
