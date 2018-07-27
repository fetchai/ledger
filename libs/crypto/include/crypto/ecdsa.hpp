#pragma once

#include "crypto/prover.hpp"
#include "crypto/verifier.hpp"
#include "crypto/ecdsa_signature.hpp"


#include <core/assert.hpp>

namespace fetch {
namespace crypto {

class ECDSAVerifier : public Verifier
{
  using PublicKey = openssl::ECDSAPublicKey<>;
  using Signature = openssl::ECDSASignature<>;

public:
  ECDSAVerifier(Identity const &ident)
    : identity_{ident}
    , public_key_{identity_.identifier()}
  {
  }

  bool Verify(byte_array_type const &data, byte_array_type const &signature) override
  {
    Signature sig {signature};
    return sig.Verify(public_key_, data);
  }

  Identity identity() override { return identity_; }

private:
  Identity identity_;
  PublicKey public_key_;
};

class ECDSASigner : public Prover
{
  using PrivateKey = openssl::ECDSAPrivateKey<>;
  using Signature = openssl::ECDSASignature<>;

  using PrivateKeyPtr = std::unique_ptr<PrivateKey>; 
  using SignaturePtr = std::unique_ptr<Signature>; 
public:
  void Load(byte_array_type const &private_key) override
  {
    SetPrivateKey(private_key);
  }

  void SetPrivateKey(byte_array_type const &private_key)
  {
     private_key_ = std::make_unique<PrivateKey>(private_key); 
  }

  void GenerateKeys()
  {
     private_key_ = std::make_unique<PrivateKey>();
  }

  bool Sign(byte_array_type const &text) final override
  {
   signature_ = std::make_unique<Signature>(Signature::Sign(*private_key_, text));
   return true;
  }

  Identity identity() final override
  { 
    return Identity(PrivateKey::ecdsa_curve_type::sn, public_key());
  }

  byte_array_type document_hash() final override { return signature_->hash(); }
  byte_array_type signature() final override { return signature_->signature(); }
  byte_array_type public_key() { return private_key_->publicKey().keyAsBin(); }
  byte_array_type private_key() { return private_key_->KeyAsBin();}

private:
  PrivateKeyPtr private_key_;
  SignaturePtr signature_;
};

}  // namespace crypto
}  // namespace fetch
