#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "crypto/ecdsa_signature.hpp"
#include "crypto/prover.hpp"
#include "crypto/verifier.hpp"

#include <core/assert.hpp>

namespace fetch {
namespace crypto {

class ECDSAVerifier : public Verifier
{
  using PublicKey = openssl::ECDSAPublicKey<>;
  using Signature = openssl::ECDSASignature<>;

public:
  ECDSAVerifier(Identity ident)
    : identity_{std::move(ident)}
    , public_key_{identity_ ? PublicKey(identity_.identifier()) : PublicKey()}
  {}

  bool Verify(byte_array_type const &data, byte_array_type const &signature) override
  {
    if (!identity_)
    {
      return false;
    }
    Signature sig{signature};
    return sig.Verify(public_key_, data);
  }

  Identity identity() override
  {
    return identity_;
  }

  operator bool() const
  {
    return identity_;
  }

private:
  Identity  identity_;
  PublicKey public_key_;
};

class ECDSASigner : public Prover
{
public:
  using PrivateKey = openssl::ECDSAPrivateKey<>;
  using Signature  = openssl::ECDSASignature<>;

  void Load(byte_array_type const &private_key) override
  {
    SetPrivateKey(private_key);
  }

  void SetPrivateKey(byte_array_type const &private_key)
  {
    private_key_ = PrivateKey(private_key);
  }

  void GenerateKeys()
  {
    private_key_ = PrivateKey();
  }

  bool Sign(byte_array_type const &text) final override
  {
    signature_ = Signature::Sign(private_key_, text);
    return true;
  }

  Identity identity() const final override
  {
    return Identity(PrivateKey::ecdsa_curve_type::sn, public_key());
  }

  byte_array_type document_hash() final override
  {
    return signature_.hash();
  }

  byte_array_type signature() final override
  {
    return signature_.signature();
  }
  byte_array_type public_key() const
  {
    return private_key_.publicKey().keyAsBin();
  }

  byte_array_type private_key()
  {
    return private_key_.KeyAsBin();
  }

  PrivateKey const &underlying_private_key() const
  {
    return private_key_;
  }

private:
  PrivateKey private_key_;
  Signature  signature_;
};

}  // namespace crypto
}  // namespace fetch
