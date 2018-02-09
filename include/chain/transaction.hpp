#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP
#include "crypto/sha256.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "serializer/byte_array_buffer.hpp"

namespace fetch {
namespace chain {

template< typename B >
class BasicTransaction {
public:
  typedef crypto::SHA256 hasher_type;
  typedef byte_array::ConstByteArray digest_type;  
  typedef B body_type;

  // TODO: Add signature list
  // TODO: Add resource list
  
  body_type const& set_body(body_type const &body) { // TODO: refactor
    body_ = body;
    serializers::ByteArrayBuffer buf;
    buf << body;
    hasher_type hash;
    hash.Reset();
    hash.Update( buf.data());
    hash.Final();
    digest_ = hash.digest();
    return body_;
  }
  
  body_type const &body() const { return body_; }
  digest_type const & digest() const { return digest_; }
private:
  body_type body_;
  digest_type digest_;

};


template< typename T, typename B >
void Serialize( T & serializer, BasicTransaction< B > const &b) {
  serializer << b.body();
}

template< typename T, typename B >
void Deserialize( T & serializer, BasicTransaction< B > &b) {
  B body;
  serializer >> body;
  b.set_body( body );
  
}

};

};
#endif
