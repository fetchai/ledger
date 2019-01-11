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

#include "http/client.hpp"

#include <stdexcept>
#include <system_error>

#include <iostream>
#include <sstream>

namespace fetch {
namespace http {

HTTPClient::HTTPClient(std::string host, uint16_t port)
  : host_(std::move(host))
  , port_(port)
{}

bool HTTPClient::Request(HTTPRequest const &request, HTTPResponse &response)
{
  // establish the connection
  if (!socket_.is_open())
  {
    if (!Connect())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to connect to server");
      return false;
    }
  }

  asio::streambuf buffer;
  request.ToStream(buffer, host_, port_);

  // send the request to the server
  std::error_code ec;
  socket_.write_some(buffer.data(), ec);
  if (ec)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to send boostrap request: ", ec.message());
    return false;
  }

  asio::streambuf input_buffer;
  std::size_t     header_length = asio::read_until(socket_, input_buffer, "\r\n\r\n", ec);
  if (ec)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to recv response header: ", ec.message());
    return false;
  }

  // work out the start of the header
  response.ParseHeader(input_buffer,
                       header_length);  // will consume header_length bytes from the buffer

  // determine if any further read is required
  std::size_t content_length = 0;
  if (response.header().Has("content-length"))
  {
    content_length = std::stoul(static_cast<std::string>(response.header()["content-length"]));
  }

  // calculate if any remaining data needs to be read
  if (input_buffer.size() < content_length)
  {
    std::size_t const remaining_length = content_length - input_buffer.size();
    asio::read(socket_, input_buffer, asio::transfer_exactly(remaining_length), ec);

    if (ec)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to recv body: ", ec.message());
      return false;
    }
  }

  // handle broken connection case
  if (input_buffer.size() < content_length)
  {
    return false;
  }

  // process the body
  response.ParseBody(input_buffer, content_length);

  // check the status code
  uint16_t const raw_status_code = static_cast<uint16_t>(response.status());

  return ((200 <= raw_status_code) && (300 > raw_status_code));
}

bool HTTPClient::Connect()
{
  using Resolver = Socket::protocol_type::resolver;

  std::error_code ec{};
  Resolver        resolver{io_service_};

  // resolve the endpoint
  Resolver::iterator endpoint = resolver.resolve(host_, std::to_string(port_), ec);
  if (ec)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to resolve host: ", ec.message());
    return false;
  }

  // establish the connection
  socket_.connect(*endpoint, ec);
  if (ec)
  {

    FETCH_LOG_WARN(LOGGING_NAME, "Unable to establish a connection: ", ec.message());
    return false;
  }

  return true;
}

}  // namespace http
}  // namespace fetch
