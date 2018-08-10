#pragma once
#include "core/byte_array/byte_array.hpp"
#include <string>

namespace fetch {
namespace byte_array {

ConstByteArray ToBase64(ConstByteArray const &str);
ConstByteArray ToHex(ConstByteArray const &str);
std::string ToHexString(ConstByteArray const &str);
ConstByteArray ToBin(ConstByteArray const &str);
std::string ToBinString(ConstByteArray const &str);
ConstByteArray ToHexReverse(ConstByteArray const &str);
ConstByteArray ToBinReverse(ConstByteArray const &str);
}  // namespace byte_array
}  // namespace fetch
