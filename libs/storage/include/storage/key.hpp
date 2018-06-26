#ifndef STORAGE_KEY_HPP
#define STORAGE_KEY_HPP
#include "core/byte_array/referenced_byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <cstring>
#include <algorithm>
namespace fetch {
namespace storage {
  
template< std::size_t S = 64 >
struct Key {
  enum {
    BLOCKS = S / 64,
    BYTES = S / 8
  };
  
  Key() {
    std::size_t i=0;
    for(; i < BLOCKS; ++i) {
      key_[i]=0;
    }
  }

  Key(byte_array::ConstByteArray const &key) {
    std::size_t i=0;
    uint8_t *ptr = reinterpret_cast< uint8_t * >( key_ );
    
    std::size_t n = std::min(std::size_t(BYTES), key.size());
    for(; i < n; ++i) {
      ptr[i] = key[i];
    }

    for(; i < BYTES; ++i) {
      ptr[i] = 0;
    }
  }
  
  int Compare(Key const &other, int &pos, int last_block, int last_bit) const {
    int i = 0;
    
    while((i < last_block) && (other.key_[i] == key_[i]))  ++i;
    
    uint64_t diff = other.key_[i] ^ key_[i];
    int bit =  __builtin_ctzl(diff);    
    if(diff == 0 ) bit = 8 *sizeof(uint64_t);
    
    if(i > last_block) {
      bit = last_bit;
    } else if(i == last_block) {
      bit =  std::min(bit, last_bit);
    }

    pos = bit + (i << 8);
    if(pos >= int(this->size())) {
      return 0;
    }

    int result = 1 - int( ((key_[i] >> (bit))<<1) & 2 );
    return result;
    
  }
  
  byte_array::ByteArray ToByteArray() const {
    byte_array::ByteArray ret;
    uint8_t const *ptr = reinterpret_cast< uint8_t const * >( key_ );
    ret.Resize( BYTES );
    for(std::size_t i=0; i < BYTES; ++i) {
      ret[i] = ptr[i];
    }
    return ret;
  }
  
  //BLOCKS
  std::size_t size() const { return BYTES << 3; }
private:
  uint64_t key_[BLOCKS];
};

}
}

#endif
