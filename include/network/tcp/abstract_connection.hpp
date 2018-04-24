#ifndef NETWORK_ABSTRACT_CONNECTION_HPP
#define NETWORK_ABSTRACT_CONNECTION_HPP

#include "network/message.hpp"

#include <memory>

namespace fetch {
namespace network {

class AbstractClientConnection {
 public:
  typedef std::shared_ptr<AbstractClientConnection> shared_type;

  virtual ~AbstractClientConnection() {}
  virtual void Send(message_type const&) = 0;

  virtual std::string Address() = 0;
};
}
}

#endif
