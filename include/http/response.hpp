#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP
#include"http/header.hpp"
#include"http/status.hpp"

#include<ostream>

namespace fetch {
namespace http {

class HTTPResponse : public std::enable_shared_from_this<HTTPResponse> {
public:
  HTTPResponse(byte_array::ByteArray body) :
    body_(body)
  {
    
  }

  byte_array::ByteArray ResponseData() const
  {

  }
  
private:
  byte_array::ByteArray header_;
  byte_array::ByteArray body_;
  
  bool keep_alive_ = false;  
};

};
};



#endif
