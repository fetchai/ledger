#ifndef BYTE_ARRAY_DECODERS_HPP
#define BYTE_ARRAY_DECODERS_HPP
#include "byte_array/referenced_byte_array.hpp"

namespace fetch {
namespace byte_array {
  
BasicByteArray FromBase64(BasicByteArray const &str) noexcept;
BasicByteArray FromHex(BasicByteArray const &str) noexcept;
  
}
}

#endif
