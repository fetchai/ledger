#ifndef NETWORK_ABSTRACT_SERVER_HPP
#define NETWORK_ABSTRACT_SERVER_HPP

#include "network/message.hpp"
#include "network/management/abstract_connection.hpp"

namespace fetch {
namespace network {

class AbstractNetworkServer {
 public:
  typedef typename AbstractConnection::connection_handle_type connection_handle_type;

  virtual void PushRequest(connection_handle_type client, message_type const& msg) = 0;

  virtual ~AbstractNetworkServer() {}
};

}
}

#endif
