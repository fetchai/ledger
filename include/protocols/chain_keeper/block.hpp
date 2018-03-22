#ifndef PROTOCOLS_SHARD_BLOCK_HPP
#define PROTOCOLS_SHARD_BLOCK_HPP
#include"byte_array/referenced_byte_array.hpp"

#include<limits>
#include<memory>

namespace fetch
{
namespace protocols 
{

struct BlockBody {     
  fetch::byte_array::ByteArray previous_hash;  
  fetch::byte_array::ByteArray transaction_hash;
  std::vector< uint32_t > groups;  
};


struct BlockMetaData {
  enum {
    UNDEFINED = uint64_t(-1)
  };

  uint64_t block_number = UNDEFINED;  
  double work       = 0;  
  double total_work = 0;
  
  bool loose_chain = true;
  bool verified = true;
};


template< typename T >
void Serialize( T & serializer, BlockBody const &body) {
  serializer << body.previous_hash << body.transaction_hash;

  serializer << body.groups;
}

template< typename T >
void Deserialize( T & serializer, BlockBody &body) {
  serializer >> body.previous_hash >> body.transaction_hash;


  serializer >> body.groups;  
}


template< typename T >
void Serialize( T & serializer, BlockMetaData const &meta) {
  serializer << meta.loose_chain << meta.verified;
  serializer << meta.block_number << meta.work << meta.total_work;    
}

template< typename T >
void Deserialize( T & serializer, BlockMetaData &meta) {
  serializer >> meta.loose_chain >> meta.verified;
  serializer >> meta.block_number >> meta.work >> meta.total_work;  
}


};
};

#endif
