#pragma once
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"

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
 * it has compared
 * all of 0xF
 *
 */
template <std::size_t S = 64>
struct Key
{
  enum
  {
    BLOCKS = S / 64,
    BYTES  = S / 8
  };

  Key() { memset(key_, 0, BYTES); }

  Key(byte_array::ConstByteArray const &key)
  {
    std::size_t i   = 0;
    uint8_t *   ptr = reinterpret_cast<uint8_t *>(key_);

    std::size_t n = std::min(std::size_t(BYTES), key.size());
    for (; i < n; ++i)
    {
      ptr[i] = key[i];
    }

    for (; i < BYTES; ++i)
    {
      ptr[i] = 0;
    }
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

    while ((i < last_block) && (other.key_[i] == key_[i])) ++i;

    uint64_t diff = other.key_[i] ^ key_[i];
    int      bit  = __builtin_ctzl(diff);
    if (diff == 0) bit = 8 * sizeof(uint64_t);

    if (i > last_block)
    {
      bit = last_bit;
    }
    else if (i == last_block)
    {
      bit = std::min(bit, last_bit);
    }

    pos = bit + (i << 8);
    if (pos >= int(this->size()))
    {
      return 0;
    }

    int result = 1 - int(((key_[i] >> (bit)) << 1) & 2);
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

    memcpy(ret.pointer(), key_, BYTES);

    return ret;
  }

  // BLOCKS
  std::size_t size() const { return BYTES << 3; }

private:
  uint64_t key_[BLOCKS];
};

}  // namespace storage
}  // namespace fetch
