#pragma once
#include "semanticsearch/index/base_types.hpp"

namespace fetch {
namespace semanticsearch {

struct ScopeIdentifier
{
  std::string            address{""};
  SemanticCoordinateType version{-1};

  bool operator<(ScopeIdentifier const &other) const
  {
    if (address == other.address)
    {
      return version < other.version;
    }

    return address < other.address;
  }

  bool operator==(ScopeIdentifier const &other) const
  {
    return (address == other.address) && (version == other.version);
  }
};

}  // namespace semanticsearch
}  // namespace fetch