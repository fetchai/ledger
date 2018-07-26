#pragma once
#include<vectorise/memory/shared_array.hpp>
#include<vectorise/platform.hpp>
#include<type_traits>
#include<initializer_list>

namespace fetch {
namespace bitmanip {
namespace details {

template< std::size_t IMPL >
class BitVectorImplementation  
{
public:
  typedef uint64_t data_type;  
  typedef memory::SharedArray< data_type > container_type;

  enum {
    LOG_BITS =  meta::Log2< 8 * sizeof( data_type ) >::value,
    BIT_MASK = (1ull << LOG_BITS) - 1,
    SIMD_SIZE = container_type::E_SIMD_COUNT
  };

  BitVectorImplementation() {
    Resize(0);
  }
  
  explicit BitVectorImplementation(std::size_t const &n) {
    Resize(n);
  }


  BitVectorImplementation(BitVectorImplementation const &other)
    : data_( other.data_.Copy() ), size_(other.size_), blocks_(other.blocks_)
  {
  }

  BitVectorImplementation(std::size_t const &size, std::initializer_list< uint64_t > const &data)
    : BitVectorImplementation(data)
  {
    assert( size <= size_ );
    size_ = size;
  }
  
  BitVectorImplementation(std::initializer_list< uint64_t > const &data) {
    data_ = container_type( data.size() );
    std::size_t i = 0;
    for(auto const &a : data)
      data_[i++] = a;
    blocks_ = data.size();
    size_ = 8 * sizeof(data_type) * blocks_;
  }

  void Resize(std::size_t const &n) {
    container_type old = data_;

    std::size_t q = n / (sizeof(data_type) * 8 ) ;
    if( (q *8*sizeof(data_type )) < n) {
      ++q;
    }
    
    data_ = container_type(q);
    blocks_ = q;
    size_ = n;
    
    SetAllZero(); // TODO: Only  set those 
    // TODO: Copy data;    
  }

  void SetAllZero() {
    data_.SetAllZero();
  }

  bool operator==(BitVectorImplementation const &other) {
    bool ret = this->size_ == other.size_;
    if(!ret) return ret;
    for(std::size_t i = 0; i < blocks_; ++i)
      ret &= (this->operator()(i) == other(i) );
    return ret;
  }

  bool operator!=(BitVectorImplementation const &other) {
    return !(this->operator==( other) );
  }

  
  BitVectorImplementation& operator^= (BitVectorImplementation const &other) {
    assert(size_ == other.size_);
    for(std::size_t i=0; i < blocks_; ++i)
      data_[i] ^= other.data_[i];
      
    return *this;
  }

  BitVectorImplementation operator^ (BitVectorImplementation const &other) const {
    assert(size_ == other.size_);
    BitVectorImplementation ret(*this);
    ret ^= other;
    return ret;
  }
  
  BitVectorImplementation& operator&= (BitVectorImplementation const &other) {
    assert(size_ == other.size_);
    for(std::size_t i=0; i < blocks_; ++i)
      data_[i] &= other.data_[i];
      
    return *this;
  }

  BitVectorImplementation operator&(BitVectorImplementation const &other) const {
    assert(size_ == other.size_);
    BitVectorImplementation ret(*this);
    ret &= other;
    return ret;
  }


  void InlineAndAssign(BitVectorImplementation const &a, BitVectorImplementation const &b) {
    for(std::size_t i=0; i < blocks_; ++i)
      data_[i] = a.data_[i] & b.data_[i];
  }
  
  
  BitVectorImplementation& operator|= (BitVectorImplementation const &other) {
    assert(size_ == other.size_);
    for(std::size_t i=0; i < blocks_; ++i)
      data_[i] |= other.data_[i];
      
    return *this;
  }
  

  BitVectorImplementation operator|(BitVectorImplementation const &other) const {
    assert(size_ == other.size_);
    BitVectorImplementation ret(*this);
    ret |= other;
    return ret;
  }

  void conditional_flip(std::size_t const &block, std::size_t const &bit, uint64_t const &base) {
    assert( (base == 1) ||(base==0) );
    data_[block] ^= base << bit;
  }

  void conditional_flip(std::size_t const &bit, uint64_t const &base) {
    conditional_flip( bit >> LOG_BITS, bit & BIT_MASK, base);
  }
  
  
  void flip(std::size_t const &block, std::size_t const &bit) {
    data_[block] ^= 1ull << bit;
  }

  void flip(std::size_t const &bit) {
    flip( bit >> LOG_BITS, bit & BIT_MASK);
  }

  data_type bit(std::size_t const &block, std::size_t const &b) const {
    return (data_[block] >> b) & 1;
  }

  data_type bit(std::size_t const &b) const {
    return bit( b >> LOG_BITS, b & BIT_MASK);
  }

  void set(std::size_t const &block, std::size_t const &bit, uint64_t const &val) {
    uint64_t mask_bit = 1ull << bit;
    data_[block] &=  ~mask_bit;
    data_[block] |= val << bit;
  }

  void set(std::size_t const &bit, uint64_t const &val) {
    set( bit >> LOG_BITS, bit & BIT_MASK, val);
  }
  
  
  data_type& operator()(std::size_t const &n) { return data_.At(n); }
  data_type const& operator()(std::size_t const &n) const { return data_.At(n); }

  std::size_t const & size() const { return size_; }
  std::size_t const & blocks() const { return blocks_; }
  container_type const & data() const { return data_;   }
  container_type & data() { return data_;   }  
private:
  container_type data_;
  std::size_t size_, blocks_;
};


template<std::size_t N >
inline int PopCount(BitVectorImplementation<N> const &n) {
  int ret = 0;
  for(std::size_t i=0; i < n.blocks(); ++i) {
    ret += __builtin_popcountl( n(i) );
  }
  return ret;
}

template<std::size_t N >
std::ostream &operator<<(std::ostream& s, BitVectorImplementation<N> const &b) {
  for(std::size_t i = 0; i < b.blocks(); ++i) {
    if(i!=0) s << " ";
    s << std::hex <<  b(i);
  }

  return s;
}
  

}

typedef details::BitVectorImplementation<0> BitVector;

  
}
}




