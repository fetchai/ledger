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

#include "semanticsearch/agent_profile.hpp"
#include "semanticsearch/index/base_types.hpp"

#include <map>

namespace fetch {
namespace semanticsearch {

class AgentDirectory
{
public:
  using AgentId        = AgentProfile::AgentId;
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using Agent          = AgentProfile::Agent;

  AgentId RegisterAgent(ConstByteArray const &pk);
  Agent   GetAgent(ConstByteArray const &pk);
  bool    UnregisterAgent(ConstByteArray const &pk);
  bool    RegisterVocabularyLocation(AgentId id, std::string model, SemanticPosition position);

private:
  AgentId                                     counter_{0};
  std::unordered_map<ConstByteArray, AgentId> pk_to_id_;
  std::map<AgentId, Agent>                    agents_;
};

}  // namespace semanticsearch
}  // namespace fetch
