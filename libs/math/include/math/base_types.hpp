#pragma once

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace math {

using SizeType   = uint64_t;
using SizeVector = std::vector<SizeType>;
using SizeSet    = std::unordered_set<SizeType>;

constexpr SizeType NO_AXIS = SizeType(-1);

}  // namespace math
}  // namespace fetch
