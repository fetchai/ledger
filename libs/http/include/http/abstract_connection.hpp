

#ifndef HTTP_ABSTRACT_CONNECTION_HPP
#define HTTP_ABSTRACT_CONNECTION_HPP
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
  typedef std::shared_ptr<AbstractHTTPConnection> shared_type;

  virtual ~AbstractHTTPConnection() {}
  virtual void        Send(HTTPResponse const &) = 0;
  virtual std::string Address()                  = 0;
};
}  // namespace http
}  // namespace fetch

#endif
