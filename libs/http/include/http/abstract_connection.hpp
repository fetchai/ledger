

#pragma once
#include "core/byte_array/byte_array.hpp"
#include "http/response.hpp"
#include "network/fetch_asio.hpp"

#include <memory>
#include <string>

namespace fetch {
namespace http {

class AbstractHTTPConnection
{
public:
  using shared_type = std::shared_ptr<AbstractHTTPConnection>;

  virtual ~AbstractHTTPConnection() {}
  virtual void        Send(HTTPResponse const &) = 0;
  virtual std::string Address()                  = 0;
};
}  // namespace http
}  // namespace fetch
