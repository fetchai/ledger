#pragma once

#include <memory>
#include <set>
#include <vector>

namespace fetch {
namespace semanticsearch {

using DBIndexType            = uint64_t;
using SemanticCoordinateType = uint64_t;  // Always internal 0 -> 1
using SemanticPosition       = std::vector<SemanticCoordinateType>;

using DBIndexList    = std::set<DBIndexType>;
using DBIndexListPtr = std::shared_ptr<DBIndexList>;

}  // namespace semanticsearch
}  // namespace fetch