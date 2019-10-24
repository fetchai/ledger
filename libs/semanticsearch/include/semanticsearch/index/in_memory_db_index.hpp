#pragma once

#include "semanticsearch/index/base_types.hpp"
#include "semanticsearch/index/database_index_interface.hpp"
#include "semanticsearch/index/semantic_subscription.hpp"
#include "semanticsearch/index/subscription_group.hpp"

#include <map>

namespace fetch {
namespace semanticsearch {

class InMemoryDBIndex : public DatabaseIndexInterface
{
public:
  void           AddRelation(SemanticSubscription const &obj) override;
  DBIndexListPtr Find(SemanticCoordinateType g, SemanticPosition position) const override;

private:
  std::map<SubscriptionGroup, DBIndexListPtr> compartments_;
  SemanticCoordinateType                      hc_width_param_start_ = 0;
  SemanticCoordinateType                      hc_width_param_end_   = 20;
};

}  // namespace semanticsearch
}  // namespace fetch