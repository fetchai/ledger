#ifndef BYTE_ARRAY_ENCODERS_HPP
#define BYTE_ARRAY_ENCODERS_HPP
#include "byte_array/referenced_byte_array.hpp"

namespace fetch {
namespace byte_array {

  BasicByteArray ToBase64(BasicByteArray const &str) ;
  BasicByteArray ToHex(BasicByteArray const &str) ;
  
};
};

#endif
