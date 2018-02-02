#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP
#include"http/header.hpp"
#include"http/status.hpp"

#include<ostream>
#include<asio.hpp>

namespace fetch {
namespace http {

class HTTPResponse : public std::enable_shared_from_this<HTTPResponse> {
public:
  HTTPResponse(byte_array::ByteArray body, Status const& status = status_code::SUCCESS_OK) :
    body_(body),
    status_(status)
  {
    
  }

  static void WriteToBuffer(HTTPResponse &res,   asio::streambuf &buffer) 
  {
    std::ostream out(&buffer);

    out <<  "HTTP/1.1 " << res.status_.code << "\r\n";
    
    if(!res.header_.Has( "content-length" ) ) {
      res.header_.Add( "content-length", res.body_.size() );
    }
    
    for(auto &field: res.header_)
    {
      out << field.first << ": " << field.second << "\r\n";
    }
    out << "\r\n";
    out.write( reinterpret_cast< char const *>(res.body_.pointer()), res.body_.size());
  }
  
private:
  byte_array::ByteArray body_;
  Status status_;
  Header header_;

  
  bool keep_alive_ = false;  
};

};
};



#endif
