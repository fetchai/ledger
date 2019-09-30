#include "agentapi/agentapi_protocol.hpp"
#include "agentapi/agentapi.hpp"

namespace fetch {
namespace agent {

AgentAPIProtocol::AgentAPIProtocol(AgentAPI *search)
  : Protocol()
{
  this->ExposeWithClientContext(REGISTER_AGENT, search, &AgentAPI::RegisterAgent);
  this->ExposeWithClientContext(UNREGISTER_AGENT, search, &AgentAPI::UnregisterAgent);

  this->ExposeWithClientContext(SEND_MESSAGE, search, &AgentAPI::SendMessage);
  this->ExposeWithClientContext(GET_MESSAGES, search, &AgentAPI::GetMessages);

  this->ExposeWithClientContext(FIND_AGENTS, search, &AgentAPI::FindAgents);
  this->ExposeWithClientContext(ADVERTISE, search, &AgentAPI::Advertise);
}

}  // namespace agent
}  // namespace fetch