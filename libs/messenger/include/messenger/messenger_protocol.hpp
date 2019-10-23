#pragma once
#include "muddle/rpc/server.hpp"

namespace fetch {
namespace messenger {
class MessengerAPI;
class MessengerProtocol : public service::Protocol
{
public:
  enum
  {
    REGISTER_MESSENGER   = 1,
    UNREGISTER_MESSENGER = 2,
    SEND_MESSAGE         = 3,
    GET_MESSAGES         = 4,
    FIND_MESSENGERS      = 5,
    ADVERTISE            = 6
  };

  MessengerProtocol(MessengerAPI *api);
};

}  // namespace messenger
}  // namespace fetch