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
#include "semanticsearch/schema/fields/object_schema_field.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"

#include <string>

namespace fetch {
namespace semanticsearch {

class VocabularyAdvertisement
{
public:
  using Vocabulary           = std::shared_ptr<ModelInstance>;
  using Index                = uint64_t;
  using ObjectSchemaFieldPtr = std::shared_ptr<ObjectSchemaField>;
  using AgentId              = uint64_t;
  using AgentIdSet           = std::set<AgentId>;
  using AgentIdSetPtr        = std::shared_ptr<AgentIdSet>;

  explicit VocabularyAdvertisement(ObjectSchemaFieldPtr const &schema)
    : schema_(schema)
    , index_{static_cast<std::size_t>(schema->rank())}
  {}

  void SubscribeAgent(AgentId aid, SemanticPosition position)
  {
    index_.AddRelation(aid, std::move(position));
  }

  AgentIdSetPtr FindAgents(SemanticPosition position, DepthParameterType depth)
  {
    return index_.Find(depth, std::move(position));
  }

  ObjectSchemaFieldPtr const &schema() const
  {
    return schema_;
  }

private:
  ObjectSchemaFieldPtr schema_;
  InMemoryDBIndex      index_;
};

}  // namespace semanticsearch
}  // namespace fetch
