#ifndef PROTOCOL_SHARD_TRANSACTION_SERIALIZER_HPP
#define PROTOCOL_SHARD_TRANSACTION_SERIALIZER_HPP
namespace fetch {
namespace serializers {

template< typename T, typename U >
void Serialize( T &serializer, fetch::chain::BasicTransaction< U > const &tx) {
  serializer << tx.body();
  // TODO: Serialize others
}

template< typename T, typename U >
void Deserialize( T &serializer, fetch::chain::BasicTransaction< U > &tx) {
  U body;
  serializer >>  body;
  tx.set_body(body);
}

};
};
#endif
