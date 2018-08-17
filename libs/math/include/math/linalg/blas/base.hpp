#pragma once

#include "vectorise/platform.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename T, uint64_t S, uint64_t I,
          uint64_t V = platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
class Blas
{
public:
  template <typename... Args>
  void operator()(Args... args) = delete;
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch
