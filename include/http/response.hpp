#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP
#include"http/header.hpp"
#include"http/status.hpp"

namespace fetch {
namespace http {

class Response : public std::enable_shared_from_this<Response>, public std::ostream {  
public:
  Response(std::shared_ptr<Session> session, long timeout_content) :
    session_(std::move(session)),
    timeout_(timeout_content)
  {
  }
  

  void Write(Status const &status = status_code::SUCCESS_OK, Header const &header) {

  }

  void Write(Status const &status, std::string const &content) {

  }


  void Write(Header const &header, std::string const &content = "") {

  }


  std::size_t size() {
    return this->size();
  }

private:
  void WriteHeader(Header const &header, std::size_t const & size) {
 
  }
  
  std::shared_ptr<Session> session_;
  uint64_t timeout_;
  bool keep_alive_ = false;  
};

};
};



#endif
