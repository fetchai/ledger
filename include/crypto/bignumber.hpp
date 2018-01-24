#ifndef CRYPTO_BIGNUMBER_HPP
#define CRYPTO_BIGNUMBER_HPP
#include"byte_array/basic_byte_array.hpp"

#include<algorithm>

namespace fetch {
namespace crypto {
  
class BigNumber : private byte_array::BasicByteArray {
public:
  typedef byte_array::ByteArray super_type;

  BigNumber() {
    Resize( sizeof(uint64_t) );
    for(std::size_t i=0; i < sizeof(uint64_t) ; ++i) {
      this->operator[](i) = 0;
    }    
  }
  
  BigNumber(BigNumber const &other) : super_type( other ) { }  
  BigNumber(super_type const &other) : super_type( other ) { }
  BigNumber(super_type const &other,
            std::size_t const &start, std::size_t const &length)
    : super_type(other, start, length) {  }
  
  BigNumber(uint64_t const& number) {
    Resize( sizeof( uint64_t ) );

    union {
      uint64_t value;
      uint8_t bytes[ sizeof(uint64_t) ];
    } data;
    data.value = number;

    // FIXME: Assumes little endian    
    for(std::size_t i = 0; i < sizeof( uint64_t ) ; ++i) {
      this->operator[]( i ) = data.bytes[i];
    }
  }

  BigNumber& operator++() {
    std::size_t i = 0;
    uint8_t val = ++this->operator[]( i ) ; 
    while( val == 0 ) {
      ++i;
      if( i == this->size() ) {
        this->Resize( i + 1 );
        val = this->operator[]( i ) = 1;
      } else {
        val = ++this->operator[]( i ) ;
      }
    }
    
    return *this;
  }

  BigNumber& operator<<=(std::size_t const &n) {
    std::size_t bits = n & 7;
    std::size_t bytes = n >> 3;
    std::size_t old = this->size();
    this->Resize( old + bytes );

    for(std::size_t i=old; i != 0; ) {
      --i;
      this->operator[](i+bytes) = this->operator[](i);
    }
    
    for(std::size_t i=0; i< bytes; ++i) {
      this->operator[](i) = 0;
    }

    std::size_t nbits = 8 - bites;
    uint8_t carry_mask = ~((1 << nbits) - 1);
    uint8_t carry = 0;    
    for(std::size_t i=0; i < size(); ++i) {

    }
  }

  uint8_t operator[](std::size_t const &n) const {
    return super_type::operator[](n);
  }
  
  bool operator<(BigNumber const& other) {
    std::size_t const N = std::min(this->size(), other.size());
    std::size_t i = 0;
    
    while( (i < N) && (this-operator[](i) == other[i]) ) 
      ++i;
    
    if(i == N) return this->size() < other.size();
    return this-operator[](i) < other[i];
  }
  
};

};
};

#endif
