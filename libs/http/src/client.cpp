#include "http/client.hpp"

#include <system_error>
#include <stdexcept>

#include <sstream>
#include <iostream>

namespace fetch {
namespace http {

HTTPClient::HTTPClient(std::string host, uint16_t port)
  : host_(std::move(host))
  , port_(port)
{
}

bool HTTPClient::Request(HTTPRequest const &request, HTTPResponse &response)
{
  // establish the connection
  if (!socket_.is_open())
  {
    if (!Connect())
    {
      fetch::logger.Warn("Failed to connect to server");
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
    fetch::logger.Warn("Failed to send boostrap request: ", ec.message());
    return false;
  }

  asio::streambuf input_buffer;
  std::size_t header_length = asio::read_until(socket_, input_buffer, "\r\n\r\n", ec);
  if (ec)
  {
    fetch::logger.Warn("Failed to recv response header: ", ec.message());
    return false;
  }

  // work out the start of the header
  char const *header_start = reinterpret_cast<char const *>(input_buffer.data().data());

#if 0
  std::cout << "Header Length: " << header_length << std::endl;
#endif

  response.ParseHeader(input_buffer, header_length); // will consume header_length bytes from the buffer

  // determine if any further read is required
  std::size_t content_length = 0;
  if (response.header().Has("content-length"))
  {
    content_length = std::stoul(
      static_cast<std::string>(
        response.header()["content-length"]
      )
    );
  }

  std::size_t const total_length = content_length + header_length;

  // calculate if any remaining data needs to be read
  if (input_buffer.size() < content_length)
  {
    std::size_t const remaining_length = content_length - input_buffer.size();

#if 0
    std::cout << " - Header length: " << header_length << std::endl;
    std::cout << " - Total length: " << total_length << std::endl;
    std::cout << " - Content Length: " << content_length << std::endl;
    std::cout << " - Buffer length: " << input_buffer.size() << std::endl;
    std::cout << " - Buffer length: " << input_buffer.data().size() << std::endl;
    std::cout << " - Remaining length: " << remaining_length << std::endl;
#endif

    std::size_t const extra_bytes = asio::read(socket_, input_buffer, asio::transfer_exactly(remaining_length), ec);

#if 0
    std::cout << " - Buffer length After: " << input_buffer.size() << std::endl;
    std::cout << " - Extra bytes: " << extra_bytes << std::endl;
#endif

    if (ec/*  && (ec != asio::error::eof)*/)
    {
      fetch::logger.Warn("Failed to recv body: ", ec.message());
      return false;
    }
  }

  // handle broken connection case
  if (input_buffer.size() < content_length)
  {
    return false;
  }

  // process the body
  char const *body_start = header_start + header_length;
  std::size_t body_length = total_length - header_length;
  std::size_t body_length2 = input_buffer.size() - header_length;

#if 0
  std::cout << "Header Length: " << header_length << std::endl;
  std::cout << "Content Length: " << content_length << std::endl;
  std::cout << "Body Length: " << body_length << std::endl;
  std::cout << "Body Length2: " << body_length2 << std::endl;
  std::cout << "Buffer Length: " << input_buffer.size() << std::endl;
//  std::cout << "== BODY START ==" << std::endl;
//  std::cout.write(body_start, body_length);
//  std::cout << "\n== BODY END ==" << std::endl;
#endif

  response.ParseBody(input_buffer, content_length);

  // check the status code
  uint16_t const raw_status_code = static_cast<uint16_t>(response.status());

  return ((200 <= raw_status_code) && (300 > raw_status_code));
}

bool HTTPClient::Connect()
{
  using Resolver = Socket::protocol_type::resolver;

  std::error_code ec{};
  Resolver resolver{io_service_};

  // resolve the endpoint
  Resolver::iterator endpoint = resolver.resolve(host_, std::to_string(port_), ec);
  if (ec)
  {
    fetch::logger.Warn("Unable to resolve host: ", ec.message());
    return false;
  }

  // establish the connection
  socket_.connect(*endpoint, ec);
  if (ec)
  {

    fetch::logger.Warn("Unable to establish a connection: ", ec.message());
    return false;
  }

  return true;
}

} // namespace http
} // namespace fetch