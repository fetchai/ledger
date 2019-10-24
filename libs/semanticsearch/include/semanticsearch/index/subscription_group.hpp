#pragma once

#include "semanticsearch/index/base_types.hpp"

namespace fetch {
namespace semanticsearch {

struct SubscriptionGroup
{
  SubscriptionGroup() = default;
  SubscriptionGroup(SemanticCoordinateType g, SemanticPosition position);

  static constexpr SemanticCoordinateType DBIndexListPtrSize(SemanticCoordinateType g)
  {
    return SemanticCoordinateType(-1) >> g;
  }

  SemanticPosition       indices;
  SemanticCoordinateType width_parameter;  ///< Parameter that determines the witdth

  bool operator<(SubscriptionGroup const &other) const;
  bool operator==(SubscriptionGroup const &other) const;
};

}  // namespace semanticsearch
}  // namespace fetch