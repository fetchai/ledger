#pragma once
#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace byte_array {

ConstByteArray ToBase64(ConstByteArray const &str);
ConstByteArray ToHex(ConstByteArray const &str);
}
}

