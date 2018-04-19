
#ifndef HTTP_ABSTRACT_CONNECTION_HPP
#define HTTP_ABSTRACT_CONNECTION_HPP

#include"http/response.hpp"

#include<string>
#include<memory>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#include <asio.hpp>
#pragma clang diagnostic pop

namespace fetch
{
namespace http
{


class AbstractHTTPConnection
{
public:
  typedef std::shared_ptr<AbstractHTTPConnection> shared_type;

  virtual ~AbstractHTTPConnection() {}
  virtual void Send(HTTPResponse const&) = 0;
  virtual std::string Address() = 0;  
};



};
};


#endif
