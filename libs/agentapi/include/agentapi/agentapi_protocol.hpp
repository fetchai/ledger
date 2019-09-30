#pragma once
#include "muddle/rpc/server.hpp"

namespace fetch {
namespace agent {
class AgentAPI;
class AgentAPIProtocol : public service::Protocol
{
public:
  enum
  {
    REGISTER_AGENT   = 1,
    UNREGISTER_AGENT = 2,
    SEND_MESSAGE     = 3,
    GET_MESSAGES     = 4,
    FIND_AGENTS      = 5,
    ADVERTISE        = 6
  };

  AgentAPIProtocol(AgentAPI *api);
};

}  // namespace agent
}  // namespace fetch