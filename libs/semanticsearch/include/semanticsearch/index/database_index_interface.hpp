#pragma once
#include "semanticsearch/index/semantic_subscription.hpp"

namespace fetch {
namespace semanticsearch {

class DatabaseIndexInterface
{
public:
  virtual ~DatabaseIndexInterface() = default;

  virtual void           AddRelation(SemanticSubscription const &obj)                    = 0;
  virtual DBIndexListPtr Find(SemanticCoordinateType g, SemanticPosition position) const = 0;
};

}  // namespace semanticsearch
}  // namespace fetch