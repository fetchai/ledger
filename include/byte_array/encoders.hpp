#ifndef BYTE_ARRAY_ENCODERS_HPP
#define BYTE_ARRAY_ENCODERS_HPP
#include "byte_array/referenced_byte_array.hpp"
#include "byte_array/details/encode_decode.hpp"
namespace fetch {
namespace byte_array {

ReferencedByteArray ToBase64(ReferencedByteArray const &str) {
  // After https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64
  ReferencedByteArray ret;
  std::size_t N = str.size();
  uint8_t const *data = reinterpret_cast<uint8_t const *>(str.pointer());
  std::size_t idx = 0;

  int invPadCount = N % 3;
  if(invPadCount == 0)
    ret.Resize((N  << 2) / 3);
  else
    ret.Resize(((N + (3 -invPadCount ) ) << 2) / 3);

  uint8_t n0, n1, n2, n3;
  for (std::size_t x = 0; x < N; x += 3) {
    uint32_t temp = static_cast<uint32_t>(data[x]) << 16;
    if ((x + 1) < N) temp += static_cast<uint32_t>(data[x + 1]) << 8;
    if ((x + 2) < N) temp += data[x + 2];

    n0 = (temp >> 18) & 63;
    n1 = (temp >> 12) & 63;
    n2 = (temp >> 6) & 63;
    n3 = temp & 63;

    ret[idx++] = details::base64chars[n0];
    ret[idx++] = details::base64chars[n1];

    if ((x + 1) < N) ret[idx++] = details::base64chars[n2];
    if ((x + 2) < N) ret[idx++] = details::base64chars[n3];
  }

  if (invPadCount > 0) 
    for (; invPadCount < 3; invPadCount++) ret[idx++] = details::base64pad;

  return ret;
}

ReferencedByteArray ToHex(ReferencedByteArray const &str) {
  uint8_t const *data = reinterpret_cast<uint8_t const *>(str.pointer());
  ReferencedByteArray ret;

  ret.Resize(str.size() << 1);
  std::size_t j = 0;
  for (std::size_t i = 0; i < str.size(); ++i) {
    uint8_t c = data[i];
    ret[j++] = details::hexChars[(c >> 4) & 0xF];
    ret[j++] = details::hexChars[c & 0xF];
  }
  return ret;
}
};
};

#endif
