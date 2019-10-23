#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <map>

class Agent;
class OefAgentEndpoint;

class Agents
{
public:
  using Mutex = std::mutex;
  using Lock = std::lock_guard<Mutex>;
  using Key = std::string;

  using AgentSP = std::shared_ptr<Agent>;
  using EndpointSP = std::shared_ptr<OefAgentEndpoint>;
  using Store = std::map<Key, AgentSP>;

  Agents()
  {
  }
  virtual ~Agents()
  {
  }

  void add(const std::string &key, std::shared_ptr<OefAgentEndpoint> endpoint);
  void remove(const std::string &key);

  std::shared_ptr<Agent> find(const std::string &key);

protected:
private:
  Store agents;
  mutable Mutex mutex;

  Agents(const Agents &other) = delete;
  Agents &operator=(const Agents &other) = delete;
  bool operator==(const Agents &other) = delete;
  bool operator<(const Agents &other) = delete;
};
