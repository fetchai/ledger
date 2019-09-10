#include "Agents.hpp"

#include "mt-core/agents/src/cpp/Agent.hpp"

void Agents::add(const std::string &key, std::shared_ptr<OefAgentEndpoint> endpoint)
{
  Lock lock(mutex);
  agents[key] = std::make_shared<Agent>(key, endpoint);
}

void Agents::remove(const std::string &key)
{
  Lock lock(mutex);
  agents.erase(key);
}

std::shared_ptr<Agent> Agents::find(const std::string &key)
{
  Lock lock(mutex);
  auto iter = agents.find(key);
  if (iter != agents.end())
  {
    return iter->second;
  }
  return std::shared_ptr<Agent>();
}
