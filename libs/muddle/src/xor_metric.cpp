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

#include "xor_metric.hpp"

#include "core/byte_array/byte_array.hpp"
#include "vectorise/platform.hpp"

#include <limits>

namespace fetch {
namespace muddle {

uint64_t CalculateDistance(Address const &from, Address const &to)
{
  uint64_t distance = std::numeric_limits<uint64_t>::max();

  if ((from.size() == to.size()) && !from.empty())
  {
    distance = CalculateDistance(from.pointer(), to.pointer(), from.size());
  }

  return distance;
}

uint64_t CalculateDistance(void const *from, void const *to, std::size_t length)
{
  using Word = uint64_t;

  static constexpr std::size_t WORD_BYTE_SIZE = sizeof(Word);
  static constexpr uint64_t    BITS_IN_BYTE   = 8;
  static constexpr uint64_t    BITS_IN_WORD   = static_cast<uint64_t>(WORD_BYTE_SIZE) * 8;
  static constexpr uint64_t    OFFSET         = BITS_IN_WORD - BITS_IN_BYTE;

  uint64_t distance = std::numeric_limits<uint64_t>::max();

  // assume maximally different
  distance = length * BITS_IN_BYTE;

  auto const *raw_from64 = reinterpret_cast<uint64_t const *>(from);
  auto const *raw_to64   = reinterpret_cast<uint64_t const *>(to);

  // the calculation will be split into two loops a 64bit aligned main loop and then for
  // completeness a loop that evaluates any trailing bytes.
  std::size_t const word_count          = length / WORD_BYTE_SIZE;
  std::size_t const trailing_byte_count = length - (word_count * WORD_BYTE_SIZE);

  // main (word) loop
  bool complete{false};
  for (std::size_t i = 0; !complete && (i < word_count); ++i)
  {
    auto const xored_value   = platform::ToBigEndian(*raw_from64++ ^ *raw_to64++);
    auto const leading_zeros = platform::CountLeadingZeroes64(xored_value);

    distance -= leading_zeros;
    complete = xored_value > 0;
  }

  if (!complete)
  {
    auto const *raw_from8 = reinterpret_cast<uint8_t const *>(raw_from64);
    auto const *raw_to8   = reinterpret_cast<uint8_t const *>(raw_to64);

    // remaining (byte) loop
    for (std::size_t i = 0; i < trailing_byte_count; ++i)
    {
      if (*raw_from8 != *raw_to8)
      {
        distance -= (platform::CountLeadingZeroes64(*raw_from8 ^ *raw_to8) - OFFSET);
        break;
      }

      distance -= BITS_IN_BYTE;

      // advance pointers
      ++raw_from8;
      ++raw_to8;
    }
  }

  return distance;
}

}  // namespace muddle
}  // namespace fetch
