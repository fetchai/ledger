#pragma once
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
#include "core/byte_array/const_byte_array.hpp"
#include "core/macros.hpp"
#include "meta/log2.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>

namespace fetch {
namespace storage {

/**
 * Key used in key value pairs. Comparing keys yields the bit position that they
 * differ.
 *
 * Note: this is done in a non standard way: bytes are compared lsb to msb even
 * though
 * conceptually this is backwards.
 *
 * So comparing 0xEF... and 0x0F... would return the fourth bit position after
 * it has compared
 * all of 0xF
 *
 */
template <std::size_t V_BITS = 256, typename BLOCK_TYPE = uint64_t>
struct Key
{
  static_assert(std::is_integral<BLOCK_TYPE>::value, "The BLOCK_TYPE must be integer type.");
  static_assert(meta::IsLog2(V_BITS) && (V_BITS >= 128),
                "Keys expected to be a cryptographic hash function output");

  using BlockType = BLOCK_TYPE;

  enum : size_t
  {
    BITS                 = V_BITS,
    BYTES                = BITS >> 3,
    BLOCK_SIZE_BITS      = sizeof(BlockType) << 3,
    BLOCK_SIZE_BITS_LOG2 = meta::Log2(BLOCK_SIZE_BITS),
    BLOCKS               = BITS >> BLOCK_SIZE_BITS_LOG2
  };

  static_assert((BITS % BLOCK_SIZE_BITS) == 0, "Key must be multiple of block size");

  using KeyArrayNative = BlockType[BLOCKS];
  using KeyArray       = std::array<BlockType, BLOCKS>;

  Key() = default;

  // TODO(private issue 957): There are a number of implicit conversions for this key, in many
  //                          places it might be a bug.
  /*explicit*/ Key(byte_array::ConstByteArray const &key)
  {
    assert(key.size() == BYTES);

    KeyArrayNative const &key_reinterpret{*reinterpret_cast<KeyArrayNative const *>(key.pointer())};
    std::copy(std::begin(key_reinterpret), std::end(key_reinterpret), std::begin(key_));
  }

  /**
   * Compare against another key
   *
   * @param: rhs The key to compare against
   *
   * @return: whether there is equality between keys
   */
  bool operator==(Key const &rhs) const
  {
    bool result = std::equal(std::begin(key_), std::end(key_), std::begin(rhs.key_));

#ifndef NDEBUG
    // Assert that the corresponding compare would return the same
    int dummy          = 0;
    int compare_result = Compare(rhs, dummy, BITS);
    assert(result == (compare_result == 0));
#endif

    return result;
  }

  /**
   * Compare against another key, returning the position of the first bit
   * difference between the
   * two, comparing byte by byte, lsb to msb (see Key comment)
   *
   * @param: other The other key to
   * @param: pos Reference to the position to set
   * @param: bits_to_compare number of bits to compare
   *
   * @return: 0, 1, or -1 for a match, more than, less than
   */
  int Compare(Key const &other, int &pos, uint16_t const bits_to_compare) const
  {
    auto const     last_block = bits_to_compare >> BLOCK_SIZE_BITS_LOG2;
    uint16_t const last_bit   = bits_to_compare & ((1 << BLOCK_SIZE_BITS_LOG2) - 1);

    uint16_t i = 0;
    while ((i < last_block) && (other.key_[i] == key_[i]))
    {
      ++i;
    }

    if (i == BLOCKS)
    {
      pos = int(BITS);
      return 0;
    }

    BlockType diff = other.key_[i] ^ key_[i];
    uint16_t  bit  = (diff == 0) ? static_cast<uint16_t>(BLOCK_SIZE_BITS)
                               : static_cast<uint16_t>(platform::CountTrailingZeroes64(diff));
    if (i == last_block)
    {
      bit = std::min(bit, last_bit);
    }

    pos = bit + (i << BLOCK_SIZE_BITS_LOG2);
    if (pos >= int(BITS))
    {
      return 0;
    }

    diff = key_[i] & (static_cast<BlockType>(1) << bit);

    int result = 1 - int((diff == 0) << 1);  // -1 == left, so this puts 'smaller numbers' left

    return result;
  }

  /**
   * Return the key as a new byte array
   *
   * @return: the byte array
   */
  byte_array::ByteArray ToByteArray() const
  {
    byte_array::ByteArray ret;
    ret.Resize(BYTES);

    KeyArrayNative &ret_reinterpret{*reinterpret_cast<KeyArrayNative *>(ret.pointer())};
    std::copy(std::begin(key_), std::end(key_), std::begin(ret_reinterpret));
    return ret;
  }

  /**
   * Return the number of bits the key represents
   *
   * @return: the number of bits
   */
  static constexpr std::size_t size_in_bits()
  {
    return BITS;
  }

private:
  KeyArray key_{};
};

}  // namespace storage
}  // namespace fetch
