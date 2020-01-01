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

#include "semanticsearch/agent_directory.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

AgentDirectory::AgentId AgentDirectory::RegisterAgent(ConstByteArray const &pk)
{
  // Checking whether the agent exists.
  auto it = pk_to_id_.find(pk);
  if (it != pk_to_id_.end())
  {
    // If so, we just return its id.
    return it->second;
  }

  // Otherwise we create a new agent id for the pk.
  AgentId id    = counter_;
  agents_[id]   = AgentProfile::New(id);
  pk_to_id_[pk] = id;

  ++counter_;
  return id;
}

AgentDirectory::Agent AgentDirectory::GetAgent(ConstByteArray const &pk)
{
  // Checking agent existance
  if (pk_to_id_.find(pk) == pk_to_id_.end())
  {
    return nullptr;
  }

  // Returning the agent details.
  auto id = pk_to_id_[pk];
  return agents_[id];
}

bool AgentDirectory::UnregisterAgent(ConstByteArray const &pk)
{
  // We treat non-register agent as successful unregisters
  auto it = pk_to_id_.find(pk);
  if (it == pk_to_id_.end())
  {
    return true;
  }

  auto id  = it->second;
  auto ait = agents_.find(id);

  // Checking that the agent entry also exists
  if (ait != agents_.end())
  {
    // We can only unregister of all vocabulary locations
    // have been unregistered.
    // TODO(private issue AEA-142): do check on number of registered locations

    // Deleting the entry
    agents_.erase(ait);
  }

  // Finally, we delete the agent from the register
  //  agents_.erase(it);

  pk_to_id_.erase(it);
  return true;
}

bool AgentDirectory::RegisterVocabularyLocation(AgentId id, std::string model,
                                                SemanticPosition position)
{
  // We can only register a vocabulary location for existing agents
  if (agents_.find(id) == agents_.end())
  {
    return false;
  }

  // The model and position is added to the agents register.
  agents_[id]->RegisterVocabularyLocation(std::move(model), std::move(position));

  return true;
}

}  // namespace semanticsearch
}  // namespace fetch
