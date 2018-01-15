#ifndef CRYPTO_PROVER_HPP
#define CRYPTO_PROVER_HPP
namespace fetch {
namespace crypto {
class Prover {
 public:
  typedef byte_array::ReferencedByteArray byte_array_type;

  virtual void SetSecret(byte_array_type const &secret) = 0;
  virtual void SetPlainText(byte_array_type const &text) = 0;
  virtual byte_array_type document_hash() = 0;
  virtual byte_array_type signature() = 0;
};
};
};

#endif
