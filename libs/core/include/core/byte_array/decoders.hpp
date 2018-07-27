#pragma once
#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace byte_array {

ConstByteArray FromBase64(ConstByteArray const &str) noexcept;
ConstByteArray FromHex(ConstByteArray const &str) noexcept;
}  // namespace byte_array
}  // namespace fetch
