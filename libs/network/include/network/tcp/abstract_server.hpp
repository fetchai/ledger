#pragma once

#include "network/management/abstract_connection.hpp"
#include "network/message.hpp"

namespace fetch {
namespace network {

class AbstractNetworkServer
{
public:
  typedef typename AbstractConnection::connection_handle_type connection_handle_type;

  virtual void PushRequest(connection_handle_type client, message_type const &msg) = 0;

  virtual ~AbstractNetworkServer() {}
};

}  // namespace network
}  // namespace fetch
