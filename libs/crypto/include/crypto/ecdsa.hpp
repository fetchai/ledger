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

#include "core/assert.hpp"
#include "core/threading/synchronised_state.hpp"
#include "crypto/ecdsa_signature.hpp"
#include "crypto/prover.hpp"
#include "crypto/verifier.hpp"

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

  ECDSASigner() = default;

  explicit ECDSASigner(ConstByteArray const &private_key)
    : private_key_{private_key}
  {}

  void Load(ConstByteArray const &private_key) override
  {
    SetPrivateKey(private_key);
  }

  void SetPrivateKey(ConstByteArray const &private_key)
  {
    private_key_.Set(PrivateKey{private_key});
  }

  void GenerateKeys()
  {
    private_key_.Set(PrivateKey{});
  }

  ConstByteArray Sign(ConstByteArray const &text) const final
  {
    ConstByteArray signature{};

    // sign the message in a thread safe way
    private_key_.Apply([&signature, &text](PrivateKey const &key) {
      signature = Signature::Sign(key, text).signature();
    });

    return signature;
  }

  Identity identity() const final
  {
    return Identity(PrivateKey::ecdsa_curve_type::sn, public_key());
  }

  ConstByteArray public_key() const
  {
    ConstByteArray public_key{};
    private_key_.Apply(
        [&public_key](PrivateKey const &key) { public_key = key.publicKey().keyAsBin(); });
    return public_key;
  }

  ConstByteArray private_key()
  {
    ConstByteArray private_key{};
    private_key_.Apply([&private_key](PrivateKey const &key) { private_key = key.KeyAsBin(); });
    return private_key;
  }

private:
  using ThreadSafePrivateKey = SynchronisedState<PrivateKey>;

  ThreadSafePrivateKey private_key_;
};

}  // namespace crypto
}  // namespace fetch
