#ifndef PROTOCOLS_SHARD_BLOCK_HPP
#define PROTOCOLS_SHARD_BLOCK_HPP
#include"byte_array/referenced_byte_array.hpp"

#include<limits>

namespace fetch
{
namespace protocols 
{

struct BlockMetaData {
  enum {
    UNDEFINED = uint64_t(-1)
  };
  
  uint64_t block_number = UNDEFINED;
  double total_work     = std::numeric_limits< double >::infinity();
  bool loose_chain = true;  
};

struct BlockBody {
  fetch::byte_array::ByteArray previous_hash;  
  fetch::byte_array::ByteArray transaction_hash;
  std::vector< uint32_t > shards;  
};


template< typename T >
void Serialize( T & serializer, BlockBody const &body) {
  serializer << body.previous_hash << body.transaction_hash << shards;
}

template< typename T >
void Deserialize( T & serializer, BlockBody &body) {
  serializer >> body.previous_hash >> body.transaction_hash >> shards;
}


template< typename T >
void Serialize( T & serializer, BlockMetaData const &meta) {
  serializer << meta.block_number << meta.total_work << meta.loose_chain; 
}

template< typename T >
void Deserialize( T & serializer, BlockMetaData &meta) {
  serializer >> meta.block_number >> meta.total_work  << meta.loose_chain;
}


};
};

#endif
