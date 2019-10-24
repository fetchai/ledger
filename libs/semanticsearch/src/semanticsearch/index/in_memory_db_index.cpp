#include "semanticsearch/index/in_memory_db_index.hpp"

namespace fetch {
namespace semanticsearch {

void InMemoryDBIndex::AddRelation(SemanticSubscription const &obj)
{
  for (SemanticCoordinateType s = hc_width_param_start_; s < hc_width_param_end_; ++s)
  {
    SubscriptionGroup idx{s, obj.position};
    auto              it = compartments_.find(idx);

    if (it == compartments_.end())
    {
      DBIndexListPtr c = std::make_shared<DBIndexList>();
      c->insert(obj.index);
      compartments_.insert({idx, c});
    }
    else
    {
      it->second->insert(obj.index);
    }
  }
}

DBIndexListPtr InMemoryDBIndex::Find(SemanticCoordinateType g, SemanticPosition position) const
{
  SubscriptionGroup idx{g, position};

  auto it = compartments_.find(idx);
  if (it == compartments_.end())
  {
    return nullptr;
  }

  return it->second;
}

}  // namespace semanticsearch
}  // namespace fetch