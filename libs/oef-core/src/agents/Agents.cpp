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

#include "oef-core/agents/Agent.hpp"
#include "oef-core/agents/Agents.hpp"

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
