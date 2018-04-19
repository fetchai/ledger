#include"byte_array/encoders.hpp"
#include "byte_array/details/encode_decode.hpp"

namespace fetch {
namespace byte_array {

BasicByteArray ToBase64(BasicByteArray const &str) {
  // After https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64

  std::size_t N = str.size();
  uint8_t const *data = reinterpret_cast<uint8_t const *>(str.pointer());
  std::size_t idx = 0;

  std::size_t invPadCount = N % 3;
  std::size_t size = ((N + (3 -invPadCount ) ) << 2) / 3;
  if(invPadCount == 0)
    size = (N  << 2) / 3;

  ByteArray ret;
  ret.Resize(size);
  
  uint8_t n0, n1, n2, n3;
  for (std::size_t x = 0; x < N; x += 3) {
    uint32_t temp = static_cast<uint32_t>(data[x]) << 16;
    if ((x + 1) < N) temp += static_cast<uint32_t>(data[x + 1]) << 8;
    if ((x + 2) < N) temp += data[x + 2];

    n0 = (temp >> 18) & 63;
    n1 = (temp >> 12) & 63;
    n2 = (temp >> 6) & 63;
    n3 = temp & 63;

    ret[idx++] = uint8_t(details::base64chars[n0]);
    ret[idx++] = uint8_t(details::base64chars[n1]);

    if ((x + 1) < N) ret[idx++] = uint8_t(details::base64chars[n2]);
    if ((x + 2) < N) ret[idx++] = uint8_t(details::base64chars[n3]);
  }

  if (invPadCount > 0) 
    for (; invPadCount < 3; invPadCount++) ret[idx++] = details::base64pad;

  return ret;
}

BasicByteArray ToHex(BasicByteArray const &str) {
  uint8_t const *data = reinterpret_cast<uint8_t const *>(str.pointer());
  ByteArray ret;
  ret.Resize(str.size() << 1);
  
  std::size_t j = 0;
  for (std::size_t i = 0; i < str.size(); ++i) {
    uint8_t c = data[i];
    ret[j++] = uint8_t(details::hexChars[(c >> 4) & 0xF]);
    ret[j++] = uint8_t(details::hexChars[c & 0xF]);
  }
  return ret;
}

};
};
