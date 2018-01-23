#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP
#incdule "crypto/sha256.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "serializer/byte_array.hpp"

namespace fetch {
namespace chain {

template< typename B >
class BasicTransaction {
public:
  typedef crypto::SHA256 hasher_type;
  typedef byte_array::ReferencedByteArray byte_array_type;  
  typedef B body_type;

  body_type const& set_body(body_const const &body) {
    body_ = body;
    serializers::ByteArrayBuffer buf;
    buf << body;
    hasher_type hash;
    hash.Reset();
    hash.Update( buf.data());
    id_ = hash.digest();
    return body_;
  }
  
  body_type const& body() const { return body_; }
  byte_array_type const &id() const { return id_; }
private:
  body_type body_;
  byte_array_type id_;

};

};
};
#endif
