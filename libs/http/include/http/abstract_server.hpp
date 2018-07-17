#ifndef HTTP_ABSTRACT_SERVER_HPP
#define HTTP_ABSTRACT_SERVER_HPP
#include "core/byte_array/byte_array.hpp"
#include "http/request.hpp"
#include "core/mutex.hpp"

namespace fetch {
namespace http {

class AbstractHTTPServer {
 public:
  typedef uint64_t handle_type;

  virtual void PushRequest(handle_type client, HTTPRequest req) = 0;

  static handle_type next_handle() {
    std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);
    handle_type ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

 private:
  static handle_type global_handle_counter_;
  static fetch::mutex::Mutex global_handle_mutex_;
};

AbstractHTTPServer::handle_type AbstractHTTPServer::global_handle_counter_ = 0;
fetch::mutex::Mutex AbstractHTTPServer::global_handle_mutex_(__LINE__,
                                                             __FILE__);
}
}

#endif
