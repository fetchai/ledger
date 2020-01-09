//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/details/encode_decode.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace fetch {
namespace byte_array {

ConstByteArray FromBase64(ConstByteArray const &str)
{
  // After
  // https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64

  // Prevents potential attack vector so
  // should be checked both in debug and release
  if ((str.size() % 4) != 0)
  {
    return {};
  }

  std::size_t pad = 0;
  while (pad < str.size() && (str[str.size() - pad - 1]) == details::base64pad)
  {
    ++pad;
  }

  ByteArray ret;
  ret.Resize(((3u * str.size()) >> 2u) - pad);

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
    if (c == details::INVALID)
    {
      throw std::runtime_error("Invalid base64 character");
    }

    buf = buf << 6u | c;

    if ((i & 3u) == 3u)
    {
      ret[j++] = (buf >> 16u) & 255u;
      ret[j++] = (buf >> 8u) & 255u;
      ret[j++] = buf & 255u;
      buf      = 0u;
    }
  }

  switch (i & 3u)
  {
  case 1:
    ret[j++] = (buf >> 4u) & 255u;
    break;
  case 2:
    ret[j++] = (buf >> 10u) & 255u;
    ret[j++] = (buf >> 2u) & 255u;
    break;
  }

  return {std::move(ret)};
}

ConstByteArray FromHex(ConstByteArray const &str)
{
  auto const *data = reinterpret_cast<char const *>(str.pointer());
  ByteArray   ret;
  ret.Resize(str.size() >> 1u);

  std::size_t n = str.size();
  std::size_t j = 0;

  for (std::size_t i = 0; i < n; i += 2)
  {
    auto next = static_cast<uint8_t>(details::DecodeHexChar(data[i]));
    if (i + 1 < n)
    {
      next = uint8_t(next << 4u);
      next = uint8_t(next | details::DecodeHexChar(data[i + 1]));
    }
    ret[j++] = next;
  }

  return {std::move(ret)};
}

}  // namespace byte_array
}  // namespace fetch
