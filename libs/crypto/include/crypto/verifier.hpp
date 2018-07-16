#ifndef CRYPTO_VERIFIER_HPP
#define CRYPTO_VERIFIER_HPP
#include "crypto/identity.hpp"

namespace fetch {
namespace crypto {
class Verifier {
 public:
  typedef byte_array::ConstByteArray byte_array_type;

  virtual Identity identity() = 0;
  virtual bool operator()(byte_array_type const &data, byte_array_type const &signature) = 0;

  
};
}
}

#endif
