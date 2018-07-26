#pragma once
#include "http/server.hpp"
namespace fetch {
namespace http {
namespace middleware {

inline typename HTTPServer::response_middleware_type AllowOrigin(
    std::string const &val) {
  return [val](fetch::http::HTTPResponse &res,
               fetch::http::HTTPRequest const &req) {
    res.header().Add("Access-Control-Allow-Origin", val);
  };
}
}
}
}

