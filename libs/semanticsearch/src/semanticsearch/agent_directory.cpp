//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "semanticsearch/agent_directory.hpp"

namespace fetch {
namespace semanticsearch {

AgentDirectory::AgentId AgentDirectory::RegisterAgent(ConstByteArray const &pk)
{
  if (pk_to_id_.find(pk) != pk_to_id_.end())
  {
    throw std::runtime_error("Agent already registered.");
  }

  AgentId id    = counter_;
  agents_[id]   = AgentProfile::New(id);
  pk_to_id_[pk] = id;

  ++counter_;
  return id;
}

AgentDirectory::Agent AgentDirectory::GetAgent(ConstByteArray const &pk)
{
  if (pk_to_id_.find(pk) == pk_to_id_.end())
  {
    return nullptr;
  }
  auto id = pk_to_id_[pk];
  return agents_[id];
}

void AgentDirectory::UnregisterAgent(AgentId /*id*/)
{
  throw std::runtime_error("Unregister not implemented yet.");
}

bool AgentDirectory::RegisterLocation(AgentId id, std::string model, SemanticPosition vector)
{
  if (agents_.find(id) == agents_.end())
  {
    return false;
  }

  agents_[id]->RegisterLocation(std::move(model), std::move(vector));

  return true;
}

}  // namespace semanticsearch
}  // namespace fetch
