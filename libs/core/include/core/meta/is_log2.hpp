#pragma once

#include "vectorise/meta/log2.hpp"

namespace fetch {
namespace meta {

template <uint64_t VALUE>
struct IsLog2
{
  static constexpr uint64_t log2_value       = Log2<VALUE>::value;
  static constexpr uint64_t calculated_value = 1u << log2_value;
  static constexpr bool     value            = (calculated_value == VALUE);
};

}  // namespace meta
}  // namespace fetch

