#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <map>
#include <memory>
#include <mutex>
#include <string>

class Agent;
class OefAgentEndpoint;

class Agents
{
public:
  using Mutex = std::mutex;
  using Lock  = std::lock_guard<Mutex>;
  using Key   = std::string;

  using AgentSP    = std::shared_ptr<Agent>;
  using EndpointSP = std::shared_ptr<OefAgentEndpoint>;
  using Store      = std::map<Key, AgentSP>;

  Agents()          = default;
  virtual ~Agents() = default;

  void add(const std::string &key, std::shared_ptr<OefAgentEndpoint> endpoint);
  void remove(const std::string &key);

  std::shared_ptr<Agent> find(const std::string &key);

protected:
private:
  Store         agents;
  mutable Mutex mutex;

  Agents(const Agents &other) = delete;
  Agents &operator=(const Agents &other)  = delete;
  bool    operator==(const Agents &other) = delete;
  bool    operator<(const Agents &other)  = delete;
};
