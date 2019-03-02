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

#include "network/muddle/network_id.hpp"

#include <iomanip>
#include <sstream>

namespace fetch {
namespace muddle {

NetworkId::NetworkId(char const id[4])
{
  static_assert(sizeof(UnderlyingType) == 4, "Function implemented with this assumption size is 4");

  value_ |= static_cast<uint32_t>(id[0]) << 24u;
  value_ |= static_cast<uint32_t>(id[1]) << 16u;
  value_ |= static_cast<uint32_t>(id[2]) << 8u;
  value_ |= static_cast<uint32_t>(id[3]);
}

std::string NetworkId::ToString() const
{
  static constexpr std::size_t BYTE_SIZE = sizeof(UnderlyingType);

  char const *id_raw = reinterpret_cast<char const *>(&value_);

#if 1

  std::ostringstream oss;
  for (std::size_t i = 0; i < BYTE_SIZE; ++i)
  {
    auto const &curr_char = id_raw[BYTE_SIZE - (i + 1)];

    if (std::isprint(curr_char))
    {
      oss << curr_char;
    }
    else
    {
      oss << std::hex << std::setfill('0') << std::setw(2) << int{curr_char} << std::dec;
    }
  }

#else

  // determine if the characters are printable ones
  bool all_printable{true};
  for (std::size_t i = 0; i < BYTE_SIZE; ++i)
  {
    if (0 == std::isprint(id_raw[i]))
    {
      all_printable = false;
      break;
    }
  }

  // format the generate the string
  std::ostringstream oss;
  if (all_printable)
  {
    // print out the characters in reverse order
    for (std::size_t i = 0; i < BYTE_SIZE; ++i)
    {
      oss << id_raw[BYTE_SIZE - (i + 1)];
    }
  }
  else
  {
    // simply print the value in hex
    oss << std::hex << std::setw(BYTE_SIZE * 2) << std::setfill('0') << value_;
  }
#endif

  return oss.str();
}

}  // namespace muddle
}  // namespace fetch
