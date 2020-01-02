#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/synchronisation/protected.hpp"
#include "crypto/ecdsa_signature.hpp"
#include "crypto/prover.hpp"
#include "crypto/verifier.hpp"

#include <utility>

namespace fetch {
namespace crypto {

class ECDSAVerifier : public Verifier
{
  using PublicKey = openssl::ECDSAPublicKey<>;
  using Signature = openssl::ECDSASignature<>;

public:
  explicit ECDSAVerifier(Identity ident)
    : identity_{std::move(ident)}
    , public_key_{identity_ ? PublicKey(identity_.identifier()) : PublicKey()}
  {}

  bool Verify(ConstByteArray const &data, ConstByteArray const &signature) override
  {
    if (!identity_)
    {
      return false;
    }

    if (signature.empty())
    {
      return false;
    }

    Signature sig{signature};
    return sig.Verify(public_key_, data);
  }

  Identity identity() const override
  {
    return identity_;
  }

  explicit operator bool() const
  {
    return static_cast<bool>(identity_);
  }

  PublicKey const &public_key() const
  {
    return public_key_;
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

  template <typename... Args>
  explicit ECDSASigner(Args &&... args)
    : private_key_{std::forward<Args>(args)...}
  {}

  void Load(ConstByteArray const &private_key) override
  {
    private_key_.ApplyVoid([&private_key](auto &key) { key = PrivateKey{private_key}; });
  }

  void GenerateKeys()
  {
    private_key_.ApplyVoid([](auto &key) { key = PrivateKey{}; });
  }

  ConstByteArray Sign(ConstByteArray const &text) const final
  {
    // sign the message in a thread safe way
    return private_key_.Apply(
        [&text](PrivateKey const &key) { return Signature::Sign(key, text).signature(); });
  }

  Identity identity() const final
  {
    return Identity(PrivateKey::EcdsaCurveType::sn, public_key());
  }

  ConstByteArray public_key() const
  {
    return private_key_.Apply([](PrivateKey const &key) { return key.PublicKey().KeyAsBin(); });
  }

  ConstByteArray private_key() const
  {
    return private_key_.Apply([](PrivateKey const &key) { return key.KeyAsBin(); });
  }

  PrivateKey::EcKeyPtr private_key_ec_key() const
  {
    return private_key_.Apply([](PrivateKey const &key) { return key.key(); });
  }

private:
  Protected<PrivateKey> private_key_;
};

}  // namespace crypto
}  // namespace fetch
