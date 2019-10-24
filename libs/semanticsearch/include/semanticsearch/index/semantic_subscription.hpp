#pragma once

#include "semanticsearch/index/base_types.hpp"

namespace fetch {
namespace semanticsearch {

struct SemanticSubscription
{
  SemanticPosition position{};
  DBIndexType      index{};
};

}  // namespace semanticsearch
}  // namespace fetch