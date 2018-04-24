#ifndef BYTE_ARRAY_DETAILS_ENCODE_DECODE_HPP
#define BYTE_ARRAY_DETAILS_ENCODE_DECODE_HPP
#include<cstddef>
#include<cstdint>

namespace fetch {
namespace byte_array {
namespace details {
 // After https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64  
  extern char const base64chars[];
  extern char const base64pad;
  extern char const hexChars[];
  extern unsigned char const base64decode[];
  
  enum {
    WHITESPACE = 64,
    EQUALS = 65,
    INVALID= 66
  };

  uint8_t DecodeHexChar(char const &c);

enum { B64_WHITESPACE = 64, B64_EQUALS = 65, B64_INVALID = 66 };  
}
}
}

#endif
