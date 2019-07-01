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

// The following was adapted from the original bitcoin code here:
//
// https://raw.githubusercontent.com/bitcoin/bitcoin/master/src/base58.cpp
//
// and with the following license
//
// Copyright (c) 2014-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fetch {
namespace byte_array {
namespace {

/** All alphanumeric characters except for "0", "I", "O", and "l" */
static const char *pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// clang-format off
static const int8_t mapBase58[256] = {
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
  -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
  22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
  -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
  47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
};
// clang-format on

bool IsSpace(uint8_t character)
{
  return character == ' ';
}

}  // namespace

// clang-format off
ConstByteArray FromBase58(ConstByteArray const &str)
{
  auto const *raw_start = str.pointer();
  auto const *raw_end   = raw_start + str.size();

  // Skip leading spaces.
  while (raw_start < raw_end && IsSpace(*raw_start))
  {
    raw_start++;
  }

  // Skip and count leading '1's.
  int zeroes = 0;
  int length = 0;
  while (raw_start < raw_end && *raw_start == '1') {
    zeroes++;
    raw_start++;
  }

  // Allocate enough space in big-endian base256 representation.
  int size = static_cast<int>((raw_end - raw_start) * 733 /1000 + 1); // log(58) / log(256), rounded up.
  std::vector<unsigned char> b256(static_cast<std::size_t>(size));

  // Process the characters.
  static_assert(sizeof(mapBase58)/sizeof(mapBase58[0]) == 256, "mapBase58.size() should be 256"); // guarantee not out of range
  while (raw_start < raw_end && !IsSpace(*raw_start)) {
    // Decode base58 character
    int carry = mapBase58[*raw_start];
    if (carry == -1)  // Invalid b58 character
    {
      return {};
    }

    int i = 0;
    for (auto it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
      carry += 58 * (*it);
      *it = static_cast<unsigned char>(carry % 256);
      carry /= 256;
    }
    assert(carry == 0);
    length = i;
    raw_start++;
  }

  // Skip leading zeroes in b256.
  auto it = b256.begin() + (size - length);
  while (it != b256.end() && *it == 0)
  {
    it++;
  }

  // Copy result into output vector.
  std::vector<uint8_t> vch;
  vch.reserve(static_cast<std::size_t>(zeroes + (b256.end() - it)));
  vch.assign(static_cast<std::size_t>(zeroes), 0x00);
  while (it != b256.end())
  {
    vch.push_back(*(it++));
  }

  return {vch.data(), vch.size()};
}

ConstByteArray ToBase58(ConstByteArray const &str)
{
  auto const *pbegin = str.pointer();
  auto const *pend = pbegin + str.size();

  // Skip & count leading zeroes.
  int zeroes = 0;
  int length = 0;
  while (pbegin != pend && *pbegin == 0) {
    pbegin++;
    zeroes++;
  }
  // Allocate enough space in big-endian base58 representation.
  int size = static_cast<int>((pend - pbegin) * 138 / 100 + 1); // log(256) / log(58), rounded up.
  std::vector<unsigned char> b58(static_cast<std::size_t>(size));
  // Process the bytes.
  while (pbegin != pend) {
    int carry = *pbegin;
    int i = 0;
    // Apply "b58 = b58 * 256 + ch".
    for (auto it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
      carry += 256 * (*it);
      *it = static_cast<unsigned char>(carry % 58);
      carry /= 58;
    }

    assert(carry == 0);
    length = i;
    pbegin++;
  }
  // Skip leading zeroes in base58 result.
  auto it = b58.begin() + (size - length);
  while (it != b58.end() && *it == 0)
  {
    it++;
  }
  // Translate the result into a string.
  std::string output;
  output.reserve(static_cast<std::size_t>(zeroes + (b58.end() - it)));
  output.assign(static_cast<std::size_t>(zeroes), '1');
  while (it != b58.end())
  {
    output += pszBase58[*(it++)];
  }

  return {output};

}
// clang-format on

}  // namespace byte_array
}  // namespace fetch
