#include "oef-core/agents/Agent.hpp"

#include "oef-core/comms/OefAgentEndpoint.hpp"

Notification::NotificationBuilder Agent::send(std::shared_ptr<google::protobuf::Message> s)
{
  return endpoint->send(s);
}

void Agent::run_sending()
{
  endpoint->run_sending();
}