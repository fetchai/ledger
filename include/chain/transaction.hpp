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
  
  body_type const& set_body(body_type const &body) {
    body_ = body;
    serializers::ByteArrayBuffer buf;
    buf << body;
    hasher_type hash;
    hash.Reset();
    hash.Update( buf.data());
    digest_ = hash.digest();
    return body_;
  }
  
  body_type const &body() const { return body_; }
  digest_type const & digest() const { return digest_; }
private:
  body_type body_;
  digest_type digest_;

};

};
};
#endif
