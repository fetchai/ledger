#ifndef HTTP_MIDDLEWARE_ALLOW_ORIGIN_HPP
#define HTTP_MIDDLEWARE_ALLOW_ORIGIN_HPP
#include"http/server.hpp"
namespace fetch
{
namespace http
{
namespace middleware
{

typename HTTPServer::response_middleware_type AllowOrigin(std::string const& val) {

  return [val](fetch::http::HTTPResponse &res, fetch::http::HTTPRequest const &req) {
    res.header().Add("Access-Control-Allow-Origin", val);
  };
}


};
};
};


#endif
