#pragma once
#include <cstddef>
#include <cstdint>

namespace fetch {
namespace vectorize {

template <typename T, std::size_t>
struct VectorInfo
{
  using naitve_type   = T;
  using register_type = T;
};
}  // namespace vectorize
}  // namespace fetch
