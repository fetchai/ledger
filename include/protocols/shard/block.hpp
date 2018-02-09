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
};

struct BlockBody {
  fetch::byte_array::ByteArray previous_hash;  
  fetch::byte_array::ByteArray transaction_hash;
};


template< typename T >
void Serialize( T & serializer, BlockBody const &body) {
  serializer << body.previous_hash << body.transaction_hash;
}

template< typename T >
void Deserialize( T & serializer, BlockBody &body) {
  serializer >> body.previous_hash >> body.transaction_hash;
}


};
};

#endif
