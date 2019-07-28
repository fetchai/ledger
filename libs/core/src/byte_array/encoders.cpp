//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/details/encode_decode.hpp"
#include "core/byte_array/encoders.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>

namespace fetch {
namespace byte_array {

ConstByteArray ToBase64(ConstByteArray const &str)
{
  if (str.size() == 0)
  {
    return {};
  }

  // After
  // https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64
  std::size_t N    = str.size();
  auto        data = reinterpret_cast<uint8_t const *>(str.pointer());
  std::size_t idx  = 0;

  std::size_t invPadCount = N % 3;
  std::size_t size        = ((N + (3 - invPadCount)) << 2) / 3;
  if (invPadCount == 0)
  {
    size = (N << 2) / 3;
  }

  ByteArray ret;
  ret.Resize(size);

  uint8_t n0, n1, n2, n3;
  for (std::size_t x = 0; x < N; x += 3)
  {
    uint32_t temp = static_cast<uint32_t>(data[x]) << 16u;
    if ((x + 1) < N)
    {
      temp += static_cast<uint32_t>(data[x + 1]) << 8u;
    }
    if ((x + 2) < N)
    {
      temp += data[x + 2];
    }

    n0 = (temp >> 18u) & 63u;
    n1 = (temp >> 12u) & 63u;
    n2 = (temp >> 6u) & 63u;
    n3 = temp & 63u;

    ret[idx++] = uint8_t(details::base64chars[n0]);
    ret[idx++] = uint8_t(details::base64chars[n1]);

    if ((x + 1) < N)
    {
      ret[idx++] = uint8_t(details::base64chars[n2]);
    }
    if ((x + 2) < N)
    {
      ret[idx++] = uint8_t(details::base64chars[n3]);
    }
  }

  if (invPadCount > 0)
  {
    for (; invPadCount < 3; invPadCount++)
    {
      ret[idx++] = uint8_t(details::base64pad);
    }
  }

  return std::move(ret);
}

ConstByteArray ToHex(ConstByteArray const &str)
{
  auto      data = reinterpret_cast<uint8_t const *>(str.pointer());
  ByteArray ret;
  ret.Resize(str.size() << 1u);

  std::size_t j = 0;
  for (std::size_t i = 0; i < str.size(); ++i)
  {
    uint8_t c = data[i];
    ret[j++]  = uint8_t(details::hexChars[(c >> 4u) & 0xF]);
    ret[j++]  = uint8_t(details::hexChars[c & 0xF]);
  }
  return std::move(ret);
}


// Reverse bits in byte
uint8_t Reverse(uint8_t c)
{
  c = uint8_t(((c & 0xF0) >> 4u) | ((c & 0x0F) << 4));
  c = uint8_t(((c & 0xCC) >> 2u) | ((c & 0x33) << 2));
  c = uint8_t(((c & 0xAA) >> 1u) | ((c & 0x55) << 1));
  return c;
}

// To hex, but with the bits in the bytes reversed endianness
ConstByteArray ToHexReverse(ConstByteArray const &str)
{
  auto      data = reinterpret_cast<uint8_t const *>(str.pointer());
  ByteArray ret;
  ret.Resize(str.size() << 1u);

  std::size_t j = 0;
  for (std::size_t i = 0; i < str.size(); ++i)
  {
    uint8_t c = data[i];
    c         = Reverse(c);
    ret[j++]  = uint8_t(details::hexChars[(c >> 4u) & 0xF]);
    ret[j++]  = uint8_t(details::hexChars[c & 0xF]);
  }
  return std::move(ret);
}

ConstByteArray ToBin(ConstByteArray const &str)
{
  auto      data = reinterpret_cast<uint8_t const *>(str.pointer());
  ByteArray ret;
  ret.Resize(str.size() * 8);

  std::size_t j = 0;
  for (std::size_t i = 0; i < str.size(); ++i)
  {
    uint8_t c = data[i];
    ret[j++]  = uint8_t(c & 0x80 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x40 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x20 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x10 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x08 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x04 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x02 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x01 ? '1' : '0');
  }
  return std::move(ret);
}

// To bin, but with the bits in the bytes reversed endianness
ConstByteArray ToBinReverse(ConstByteArray const &str)
{
  auto      data = reinterpret_cast<uint8_t const *>(str.pointer());
  ByteArray ret;
  ret.Resize(str.size() * 8);

  std::size_t j = 0;
  for (std::size_t i = 0; i < str.size(); ++i)
  {
    uint8_t c = data[i];
    c         = Reverse(c);
    ret[j++]  = uint8_t(c & 0x80 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x40 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x20 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x10 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x08 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x04 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x02 ? '1' : '0');
    ret[j++]  = uint8_t(c & 0x01 ? '1' : '0');
  }
  return std::move(ret);
}

}  // namespace byte_array
}  // namespace fetch
