#pragma once
#include "semanticsearch/index/base_types.hpp"

#include <memory>
#include <string>

namespace fetch {
namespace semanticsearch {

struct Location
{
  std::string      model;
  SemanticPosition position;
  bool             operator<(Location const &other) const
  {

    if (model == other.model)
    {
      std::size_t i = 0, n = std::min(position.size(), other.position.size());
      while ((i < n) && (position[i] == other.position[i]))
      {
        ++i;
      }

      if (i >= n)
      {
        return false;
      }

      return position[i] < other.position[i];
    }

    return model < other.model;
  }
};

}  // namespace semanticsearch
}  // namespace fetch