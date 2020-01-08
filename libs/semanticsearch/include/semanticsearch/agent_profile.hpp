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

#include "semanticsearch/index/base_types.hpp"
#include "semanticsearch/location.hpp"

#include <crypto/identity.hpp>
#include <memory>
#include <string>

namespace fetch {
namespace semanticsearch {

struct AgentProfile
{
  using Identity = fetch::crypto::Identity;
  using Agent    = std::shared_ptr<AgentProfile>;
  using AgentId  = uint64_t;

  static Agent New(AgentId id)
  {
    return Agent(new AgentProfile(id));
  }

  void RegisterVocabularyLocation(std::string model, SemanticPosition position)
  {
    VocabularyLocation loc;

    loc.model    = std::move(model);
    loc.position = std::move(position);

    locations.insert(std::move(loc));
  }

  Identity                     identity;
  AgentId                      id;
  std::set<VocabularyLocation> locations;

private:
  explicit AgentProfile(AgentId i)
    : id(i)
  {}
};

using Agent = AgentProfile::Agent;

}  // namespace semanticsearch
}  // namespace fetch
