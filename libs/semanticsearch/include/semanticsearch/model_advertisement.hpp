#pragma once

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

  VocabularyAdvertisement(VocabularySchema object_model)
    : object_model_(std::move(object_model))
  {}

  void SubscribeAgent(AgentId aid, SemanticPosition position)
  {
    SemanticSubscription rel;
    rel.position = std::move(position);
    rel.index    = aid;  // TODO: Change to agent id

    index_.AddRelation(rel);
  }

  AgentIdSet FindAgents(SemanticPosition position, SemanticCoordinateType granularity)
  {
    return index_.Find(std::move(granularity), std::move(position));
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