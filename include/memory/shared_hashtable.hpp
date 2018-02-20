#ifndef MEMORY_SHARED_SHAREDHASHTABLE_HPP
#define MEMORY_SHARED_SHAREDHASHTABLE_HPP
#include"byte_array/basic_byte_array.hpp"
#include"memory/shared_array.hpp"
#include"assert.hpp"


#include <stdlib.h> 
#include<memory>
namespace fetch
{
namespace memory
{

namespace details {

template<typename T >
struct Record
{
  uint64_t full_hash = 0;     
  byte_array::BasicByteArray key;
  T value;
};

};



template< typename T >
class SharedHashTable : private SharedArray< details::Record< T > >
{
public:
  typedef SharedArray< details::Record< T > > super_type;
  
  typedef T type;
  typedef details::Record< T > record_type;
   
  typedef std::shared_ptr< std::size_t > shared_size_type;
  typedef std::shared_ptr< record_type > shared_data_type;

  enum {
    NO_CAPACITY = uint32_t(-1)    
  };
  
    
  
  SharedHashTable(std::size_t n) :
    super_type( 1 << ( n ) ),
    mask_( ( 1 << n ) - 1 ) {
  }
  
  SharedHashTable() : SharedHashTable(0) {}
  SharedHashTable(SharedHashTable const &other)
    : super_type(other),
      mask_(other.mask_) {
  }

  SharedHashTable(SharedHashTable &&other)
    : super_type(other) {
    std::swap(mask_, other.mask_);
  }
  
  SharedHashTable &operator=(SharedHashTable &&other) {
    super_type::operator=(other);
    std::swap(mask_, other.mask_);
    return *this;
  }

  ~SharedHashTable() { }

  
  type& operator[](byte_array::BasicByteArray const& key) 
  {
    uint32_t hash;
    uint32_t index =  Find( key, hash );
    assert( index != uint32_t( -1 ) );
    if(index == uint32_t( -1 ) ) {
      return super_type::At(0).value;      
    }
    
    record_type &data = super_type::At( index );
    if(data.key.size() == 0 ) {
      data.full_hash = hash;      
      data.key = key;
    }
    
    return data.value;
  }

  type const& operator[](byte_array::BasicByteArray const& key) const
  {
    return super_type::At( Find( key ) ).value;    
  }

  type& At(uint32_t const& index) 
  {
    return super_type::At(index).value;    
  }

  type const& At(uint32_t const& index) const
  {
    return super_type::At(index).value;    
  }  
  
  bool HasCapacityFor(byte_array::BasicByteArray const& key) const
  {
    return Find( key ) != NO_CAPACITY ;    
  }  

  uint32_t Find( byte_array::BasicByteArray const& key) 
  {
    uint32_t hash;
    return Find(hash);
  }
  
  uint32_t Find( byte_array::BasicByteArray const& key, uint32_t &hash) 
  {
    // TODO: Make template class
    hash = 2166136261;
    for (std::size_t i = 0; i < key.size(); ++i)
    {
      hash = (hash * 16777619) ^ key[i];
    }
    
    uint32_t index = hash & mask_;    
    auto &d = super_type::At(index);

    if(d.full_hash != hash) {
      return NO_CAPACITY ;
    }
    else
    {      
      if(d.key != key )
      {
        return NO_CAPACITY ;
      }
    }
    
    return index; 
  }
  
private:


  

  uint32_t mask_;
};



};
};


#endif
