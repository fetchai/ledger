#pragma once
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <algorithm>
#include <cstring>
namespace fetch {
namespace storage {

/**
 * Key used in key value pairs. Comparing keys yields the bit position that they differ.
 *
 * Note: this is done in a non standard way: bytes are compared lsb to msb even though
 * conceptually this is backwards.
 *
 * So comparing 0xEF... and 0x0F... would return the fourth bit position after it has compared
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
    assert(key.size() == BYTES);

    // Force the byte array to be packed left to right
    for (std::size_t i = 0; i < BLOCKS; ++i)
    {
      uint64_t res = 0;
      res |= uint64_t(key[(i*8) + 0]) << 56;
      res |= uint64_t(key[(i*8) + 1]) << 48;
      res |= uint64_t(key[(i*8) + 2]) << 40;
      res |= uint64_t(key[(i*8) + 3]) << 32;
      res |= uint64_t(key[(i*8) + 4]) << 24;
      res |= uint64_t(key[(i*8) + 5]) << 16;
      res |= uint64_t(key[(i*8) + 6]) << 8;
      res |= uint64_t(key[(i*8) + 7]);
      key_[i] = res;
    }
  }

  /**
   * Compare against another key, returning the position of the first bit difference between the
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
    int      bit  = __builtin_clzl(diff);
    if (diff == 0) bit = 8 * sizeof(uint64_t);

    if (i == last_block)
    {
      bit = std::min(bit, last_bit);
    }

    pos = bit + (i << 8);
    if (pos >= int(this->size()))
    {
      return 0;
    }

    diff = key_[0] & (1ull << 63) >> bit;

    int result = 1 - int((diff == 0) << 1); // -1 == left, so this puts 'smaller numbers' left

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

    for (std::size_t i = 0; i < BLOCKS; ++i)
    {
      ret[0 +(i*8)] = (key_[i] >> 56) & 0xFF;
      ret[1 +(i*8)] = (key_[i] >> 48) & 0xFF;
      ret[2 +(i*8)] = (key_[i] >> 40) & 0xFF;
      ret[3 +(i*8)] = (key_[i] >> 32) & 0xFF;
      ret[4 +(i*8)] = (key_[i] >> 24) & 0xFF;
      ret[5 +(i*8)] = (key_[i] >> 16) & 0xFF;
      ret[6 +(i*8)] = (key_[i] >> 8) & 0xFF;
      ret[7 +(i*8)] = (key_[i]) & 0xFF;
    }

    return ret;
  }

  // BLOCKS
  std::size_t size() const { return BYTES << 3; }

private:
  uint64_t key_[BLOCKS];
};

}  // namespace storage
}  // namespace fetch
