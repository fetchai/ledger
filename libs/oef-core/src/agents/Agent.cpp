#include "Agent.hpp"

#include "mt-core/comms/src/cpp/OefAgentEndpoint.hpp"

Notification::NotificationBuilder Agent::send(std::shared_ptr<google::protobuf::Message> s)
{
  return endpoint -> send(s);
}


void Agent::run_sending()
{
  endpoint->run_sending();
}