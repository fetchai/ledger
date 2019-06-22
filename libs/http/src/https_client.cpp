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

#include "core/logger.hpp"
#include "http/https_client.hpp"
#include "network/fetch_asio.hpp"

#include <utility>

namespace fetch {
namespace http {

/**
 * Construct an HTTPS client targetted at a specified host
 *
 * @param host The host or IP address of the server
 * @param port The port to establish the connection on
 */
HttpsClient::HttpsClient(std::string host, uint16_t port)
  : HttpClient{std::move(host), port}
{
  context_.set_default_verify_paths();
  socket_.set_verify_mode(asio::ssl::verify_peer);
}

/**
 * Establish the connection to the remote server
 *
 * @return true if successful, otherwise false
 */
bool HttpsClient::Connect()
{
  using Resolver = Socket::protocol_type::resolver;

  std::error_code ec{};
  Resolver        resolver{io_service_};

  // resolve the endpoint
  Resolver::iterator endpoint = resolver.resolve(host(), std::to_string(port()), ec);
  if (ec)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to resolve host: ", ec.message());
    return false;
  }

  // establish the connection
  socket_.lowest_layer().connect(*endpoint, ec);
  if (ec)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to establish a connection: ", ec.message());
    return false;
  }

  // to the SSL handshake
  socket_.handshake(asio::ssl::stream_base::client, ec);
  if (ec)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Handshake failed: ", ec.message());
    return false;
  }

  return true;
}

/**
 * Write the contents of the buffer to the socket
 *
 * @param buffer The input buffer to send
 * @param ec The output error code from the operation
 */
void HttpsClient::Write(asio::streambuf const &buffer, std::error_code &ec)
{
  socket_.write_some(buffer.data(), ec);
}

/**
 * Read data from the buffer until a specific delimiter is reached
 *
 * @param buffer The buffer to populate
 * @param delimiter The target delimiter
 * @param ec The output error code from the operation
 * @return The number of bytes read from the stream
 */
std::size_t HttpsClient::ReadUntil(asio::streambuf &buffer, char const *delimiter,
                                   std::error_code &ec)
{
  return asio::read_until(socket_, buffer, delimiter, ec);
}

/**
 * Read an exact amount of data from the socket
 *
 * @param buffer The buffer to populate
 * @param length The number of bytes to be read
 * @param ec The output error code from the operation
 */
void HttpsClient::ReadExactly(asio::streambuf &buffer, std::size_t length, std::error_code &ec)
{
  asio::read(socket_, buffer, asio::transfer_exactly(length), ec);
}

}  // namespace http
}  // namespace fetch
