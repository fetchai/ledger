#include "core/byte_array/decoders.hpp"
#include "core/assert.hpp"
#include "core/byte_array/details/encode_decode.hpp"

namespace fetch {
namespace byte_array {

ConstByteArray FromBase64(ConstByteArray const &str) noexcept
{
  // After
  // https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64
  assert((str.size() % 4) == 0);
  std::size_t pad = 0;
  while ((pad < str.size()) && (str[str.size() - pad - 1] == details::base64pad)) ++pad;

  ByteArray ret;
  ret.Resize(((3 * str.size()) >> 2) - pad);

  std::size_t j   = 0;
  uint32_t    buf = 0;
  std::size_t i   = 0;

  for (; i < str.size(); ++i)
  {
    uint8_t c = details::base64decode[str[i]];

    if (c == details::EQUALS)
    {
      --i;
      break;
    }
    else if (c == details::INVALID)
    {
      return ConstByteArray();
    }

    buf = buf << 6 | c;

    if ((i & 3) == 3)
    {
      ret[j++] = (buf >> 16) & 255;
      ret[j++] = (buf >> 8) & 255;
      ret[j++] = buf & 255;
      buf      = 0;
    }
  }

  switch (i & 3)
  {
  case 1:
    ret[j++] = (buf >> 4) & 255;
    break;
  case 2:
    ret[j++] = (buf >> 10) & 255;
    ret[j++] = (buf >> 2) & 255;
    break;
  }

  return ret;
}

ConstByteArray FromHex(ConstByteArray const &str) noexcept
{
  char const *data = reinterpret_cast<char const *>(str.pointer());
  ByteArray   ret;
  ret.Resize(str.size() >> 1);

  std::size_t n = str.size();
  std::size_t j = 0;
  try
  {

    for (std::size_t i = 0; i < n; i += 2)
    {
      uint8_t next = uint8_t(details::DecodeHexChar(data[i]));
      if (i + 1 < n)
      {
        next = uint8_t(next << 4);
        next = uint8_t(next | details::DecodeHexChar(data[i + 1]));
      }
      ret[j++] = next;
    }
  }
  catch (std::runtime_error const &)
  {
    return ConstByteArray();
  }

  return ret;
}

}  // namespace byte_array
}  // namespace fetch
