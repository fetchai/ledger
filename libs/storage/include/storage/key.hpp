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
    //debugKey = key.Copy();

    memset(key_, 0, BYTES);

    //std::cout << "Setting key!" << std::endl;

    for (std::size_t i = 0; i < key.size(); ++i)
    {
      //std::cout << "write: " << (i >> 3) << std::endl;

      key_[i >> 3] |= uint64_t(key[i]) << (56 - (8 *(i % 8)));
    }

    //std::cout << "~~" << std::endl;
    //std::cout << ToHex(key) << std::endl;
    //std::cout << std::hex;

    for (std::size_t i = 0; i < BLOCKS; ++i)
    {
      //std::cout << key_[i] ;
    }
    //std::cout << std::dec << std::endl;
    //std::cout << "##" << std::endl;

    verify();
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
    //last_bit = 999;

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

    //uint64_t top_bit = uint64_t(1) << 63;
    uint64_t top_bit = 0x8000000000000000;
    //std::cout << std::hex << top_bit << std::dec << std::endl;
    top_bit >>= bit;

    diff = key_[0] & top_bit;
    //std::cout << std::hex << diff << std::dec << std::endl;

    int result = 1 - int((diff == 0) << 1); // -1 == left, so this puts 'smaller numbers' left

    //std::cout << "RRR: " << result << std::endl;
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

    for (std::size_t i = 0; i < BYTES;i++)
    {
      std::size_t key_index     = i >> 3;
      std::size_t sub_key_index = i % 8;
      uint64_t temp = key_[key_index];
      ret[i] = (temp >> (56 -((sub_key_index) * 8))) & 0xff;
    }


    return ret;
  }

  void verify()
  {

//    auto ret = ToByteArray();
//
//    byte_array::ByteArray alt;
//    alt.Resize(BYTES);
//
//    std::size_t j = 0;
//    for (std::size_t i = 0; i < BLOCKS;i++)
//    {
//      uint64_t temp = key_[i];
//      alt[j++] = (temp & 0xff00000000000000) >> 56; temp <<= 8;
//      alt[j++] = (temp & 0xff00000000000000) >> 56; temp <<= 8;
//      alt[j++] = (temp & 0xff00000000000000) >> 56; temp <<= 8;
//      alt[j++] = (temp & 0xff00000000000000) >> 56; temp <<= 8;
//      alt[j++] = (temp & 0xff00000000000000) >> 56; temp <<= 8;
//      alt[j++] = (temp & 0xff00000000000000) >> 56; temp <<= 8;
//      alt[j++] = (temp & 0xff00000000000000) >> 56; temp <<= 8;
//      alt[j++] = (temp & 0xff00000000000000) >> 56; temp <<= 8;
//    }
//
//    //std::cout << "bonus" << std::endl;
//    //std::cout << ToHex(alt) << std::endl;
//    //std::cout << ToHex(ret) << std::endl;
//
//    if(debugKey != ret)
//    {
//      //std::cout << "Keys not the same!" << std::endl;
//
//      //std::cout << ToBin(debugKey) << std::endl;
//      //std::cout << ToBin(ret) << std::endl;
//      //std::cout << "" << std::endl;
//      //std::cout << ToHex(debugKey) << std::endl;
//      //std::cout << ToHex(ret) << std::endl;
//      //std::cout << std::hex << key_[0] << std::endl;
//      //std::cout << "" << std::endl;
//
//      exit(1);
//    }
//    else
//    {
//      //std::cout << "keys same" << std::endl;
//    }
  }

  // BLOCKS
  std::size_t size() const { return BYTES << 3; }

private:
  uint64_t                   key_[BLOCKS];
  //byte_array::ConstByteArray debugKey;
};

}  // namespace storage
}  // namespace fetch
