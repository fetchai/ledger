#ifndef NETWORK_ABSTRACT_SERVER_HPP
#define NETWORK_ABSTRACT_SERVER_HPP

#include "network/message.hpp"

#include <mutex>

namespace fetch {
namespace network {

class AbstractNetworkServer {
 public:
  typedef uint64_t handle_type;

  virtual void PushRequest(handle_type client, message_type const& msg) = 0;

  static handle_type next_handle() {
    std::lock_guard<std::mutex> lck(global_handle_mutex_);
    handle_type ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

 private:
  static handle_type global_handle_counter_;
  static std::mutex global_handle_mutex_;
};

AbstractNetworkServer::handle_type
    AbstractNetworkServer::global_handle_counter_ = 0;
std::mutex AbstractNetworkServer::global_handle_mutex_;
};
};

#endif
