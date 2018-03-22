#ifndef PROTOCOLS_CHAIN_KEEPER_BLOCK_HPP
#define PROTOCOLS_CHAIN_KEEPER_BLOCK_HPP
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
