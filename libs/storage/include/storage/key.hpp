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
#include "vectorise/platform.hpp"

#include <algorithm>
#include <cstring>
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
 * it has compared all of 0xF
 *
 */
template <std::size_t BITS = 256>
struct Key
{
  enum
  {
    BLOCKS = BITS / 64,
    BYTES  = BITS / 8
  };

  Key()
  {
    memset(key_, 0, BYTES);
  }

  Key(byte_array::ConstByteArray const &key)
  {
    static_assert(BITS == 128 || BITS == 256 || BITS >= 1024,
                  "Keys expected to be a cryptographic hash function output");
    assert(key.size() == BYTES);

    const uint64_t *key_reinterpret = reinterpret_cast<const uint64_t *>(key.pointer());

    // Force the byte array to fill the 64 bit key from 'left to right'
    for (std::size_t i = 0; i < BLOCKS; ++i)
    {
      // TODO(private issue 459): This actually causes an inconsistency with what is written to the
      //                          disk. This should be investigated.
      key_[i] = platform::ConvertToBigEndian(key_reinterpret[i]);
    }
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
    bool result = true;

    for (std::size_t i = 0; i < BLOCKS; ++i)
    {
      if (key_[i] != rhs.key_[i])
      {
        result = false;
        break;
      }
    }

    // Assert that the corresponding compare would return the same
    int dummy          = 0;
    int compare_result = Compare(rhs, dummy, 0, 64);

    FETCH_UNUSED(compare_result);
    /* assert(result == compare_result); */  // TODO(HUT): look into this

    return result;
  }

  /**
   * Compare against another key, returning the position of the first bit
   * difference between the
   * two, comparing byte by byte, lsb to msb (see Key comment)
   *
   * @param: other The other key to
   * @param: pos Reference to the position to set
   * @param: last_block Last block to compare
   * @param: last_bit Last bit to compare
   *
   * @return: 0, 1, or -1 for a match, more than, less than
   */
  int Compare(Key const &other, int &pos, int last_block, int last_bit) const
  {
    int i = 0;

    while ((i < last_block) && (other.key_[i] == key_[i]))
    {
      ++i;
    }

    uint64_t diff = other.key_[i] ^ key_[i];
    int      bit  = platform::CountLeadingZeroes64(diff);
    if (diff == 0)
    {
      bit = 8 * sizeof(uint64_t);
    }

    if (i == last_block)
    {
      bit = std::min(bit, last_bit);
    }

    pos = bit + (i << 8);
    if (pos >= int(this->size_in_bits()))
    {
      return 0;
    }

    diff = key_[i] & (1ull << 63) >> bit;

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

    uint64_t *ret_reinterpret = reinterpret_cast<uint64_t *>(ret.pointer());

    // Force the byte array to fill the 64 bit key from 'left to right'
    for (std::size_t i = 0; i < BLOCKS; ++i)
    {
      ret_reinterpret[i] = platform::ConvertToBigEndian(key_[i]);
    }

    return ret;
  }

  /**
   * Return the number of bits the key represents
   *
   * @return: the number of bits
   */
  std::size_t size_in_bits() const
  {
    return BITS;
  }

private:
  uint64_t key_[BLOCKS];
};

}  // namespace storage
}  // namespace fetch
