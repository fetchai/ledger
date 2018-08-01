#pragma once
#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace byte_array {

ConstByteArray ToBase64(ConstByteArray const &str);
ConstByteArray ToHex(ConstByteArray const &str);
ConstByteArray ToBin(ConstByteArray const &str);
ConstByteArray ToHexReverse(ConstByteArray const &str);
ConstByteArray ToBinReverse(ConstByteArray const &str);
}  // namespace byte_array
}  // namespace fetch
