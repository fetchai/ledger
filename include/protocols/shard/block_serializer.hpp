#ifndef PROTOCOL_SHARD_BLOCK_SERIALIZER_HPP
#define PROTOCOL_SHARD_BLOCK_SERIALIZER_HPP
#include"protocols/shard/block.hpp"

namespace fetch {
namespace serializers {

template< typename T >
T& Serialize( T & serializer, BlockBody const &body) {
  serializer << body.previous_hash << body.transaction_hash;
  return serializer;
}

template< typename T >
T& Deserialize( T & serializer, BlockBody const &body) {
  serializer >> body.previous_hash >> body.transaction_hash;
  return serializer;  
}

};
};

#endif
