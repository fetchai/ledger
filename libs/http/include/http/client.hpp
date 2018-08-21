#ifndef FETCH_CLIENT_HPP
#define FETCH_CLIENT_HPP

#include "core/byte_array/byte_array.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "network/fetch_asio.hpp"

#include <cstdint>
#include <string>

namespace fetch {
namespace http {

/**
 * Simple blocking HTTP client used for querying information
 */
class HTTPClient
{
public:

  static constexpr char const *LOGGING_NAME = "HTTPClient";

  // Construction / Destruction
  explicit HTTPClient(std::string host, uint16_t port = 80);
  ~HTTPClient() = default;

  bool Request(HTTPRequest const &request, HTTPResponse &response);

private:
  using IoService = asio::io_service;
  using Socket    = asio::ip::tcp::socket;

  bool Connect();

  std::string host_;
  uint16_t    port_;
  IoService   io_service_;
  Socket      socket_{io_service_};
};

}  // namespace http
}  // namespace fetch

#endif  // FETCH_CLIENT_HPP
