#pragma once
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
#include "core/random/lfg.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "storage/resource_mapper.hpp"

#include <unordered_set>

namespace fetch {
namespace testing {

// Since the stacks do not serialize, use this class to make testing using strings easier
class StringProxy
{
public:
  StringProxy()
    : string_as_chars{}
  {
    memset(string_as_chars, 0, sizeof string_as_chars);
  }

  explicit StringProxy(std::string const &in)
    : string_as_chars{}
  {
    memset(string_as_chars, 0, sizeof string_as_chars);
    std::memcpy(string_as_chars, in.c_str(), std::min(in.size(), std::size_t(127)));
  }

  bool operator==(StringProxy const &rhs) const
  {
    return memcmp(string_as_chars, rhs.string_as_chars, 128) == 0;
  }

  bool operator!=(StringProxy const &rhs) const
  {
    return !operator==(rhs);
  }

  char string_as_chars[128];
};

inline std::ostream &operator<<(std::ostream &os, StringProxy const &m)
{
  return os << m.string_as_chars;
}

// Generate unique hashes that are very close together to stress unit tests. Do this by creating a
// random reference and then for each hash required flipping a single bit of this reference hash
inline std::unordered_set<fetch::byte_array::ByteArray> GenerateUniqueHashes(uint64_t size,
                                                                             uint64_t seed = 0)
{
  random::LinearCongruentialGenerator lcg(seed);
  fetch::byte_array::ByteArray reference = crypto::Hash<crypto::SHA256>(std::to_string(lcg()));
  std::unordered_set<fetch::byte_array::ByteArray> ret;
  ret.reserve(size);
  uint32_t bit_flip_position      = 0;
  uint32_t byte_flip_position     = 0;
  uint8_t  sub_byte_flip_position = 0;
  assert(reference.size() == 256 / 8);

  while (ret.size() < size)
  {
    // Copy reference
    fetch::byte_array::ByteArray to_push = reference.Copy();

    // Flip one bit
    byte_flip_position          = bit_flip_position >> 3u;
    sub_byte_flip_position      = uint8_t(1ull << (bit_flip_position & 0x7u));
    to_push[byte_flip_position] = to_push[byte_flip_position] ^ sub_byte_flip_position;

    ret.emplace(to_push);

    bit_flip_position++;

    // once all bits are flipped, generate new reference
    if (bit_flip_position == 256)
    {
      bit_flip_position = 0;
      reference         = crypto::Hash<crypto::SHA256>(std::to_string(lcg()));
    }
  }

  return ret;
}

// Convenience function - generate unique IDs
inline std::unordered_set<storage::ResourceID> GenerateUniqueIDs(uint64_t size, uint64_t seed = 0)
{
  std::unordered_set<storage::ResourceID> ret;
  auto                                    hashes = GenerateUniqueHashes(size, seed);

  for (auto const &i : hashes)
  {
    byte_array::ConstByteArray as_const{i};
    ret.emplace(as_const);
  }

  return ret;
}

}  // namespace testing
}  // namespace fetch
