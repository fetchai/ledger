#pragma once
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

#include "semanticsearch/index/in_memory_db_index.hpp"
#include "semanticsearch/schema/properties_map.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"

#include <string>

namespace fetch {
namespace semanticsearch {

class VocabularyAdvertisement
{
public:
  using Vocabulary       = std::shared_ptr<VocabularyInstance>;
  using Index            = uint64_t;
  using VocabularySchema = std::shared_ptr<PropertiesToSubspace>;
  using AgentId          = uint64_t;
  using AgentIdSet       = std::shared_ptr<std::set<AgentId>>;

  explicit VocabularyAdvertisement(VocabularySchema object_model)
    : object_model_(std::move(object_model))
  {}

  void SubscribeAgent(AgentId aid, SemanticPosition position)
  {
    SemanticSubscription rel;
    rel.position = std::move(position);
    rel.index    = aid;  // TODO(private issue AEA-129): Change to agent id

    index_.AddRelation(rel);
  }

  AgentIdSet FindAgents(SemanticPosition position, SemanticCoordinateType granularity)
  {
    return index_.Find(granularity, std::move(position));
  }

  VocabularySchema model() const
  {
    return object_model_;
  }

private:
  VocabularySchema object_model_;
  InMemoryDBIndex  index_;
};

}  // namespace semanticsearch
}  // namespace fetch
