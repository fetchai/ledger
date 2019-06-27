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

#include "core/byte_array/const_byte_array.hpp"
#include "core/threading/synchronised_state.hpp"
#include "crypto/ecdsa_signature.hpp"
#include "crypto/prover.hpp"
#include "crypto/signature_register.hpp"
#include "crypto/verifier.hpp"

#include "fetch_bls.hpp"

#include <atomic>
#include <sstream>
#include <utility>

namespace fetch {
namespace crypto {

namespace details {
// TODO: This entire file should be updated using the `bls_base.hpp` instead.
  
struct BLSInitialiser
{
  BLSInitialiser()
  {
    bool a{true};
    a = was_initialised.exchange(a);
    if (!a)
    {
      // use BN254
      bls::init();
    }
  }

  static std::atomic<bool> was_initialised;
};

std::atomic<bool> BLSInitialiser::was_initialised{false};
}  // namespace details

class BLSVerifier : public Verifier
{
public:
  explicit BLSVerifier(Identity ident)
    : identity_{std::move(ident)}
  {
    details::BLSInitialiser();

    std::stringstream buffer(static_cast<std::string>(identity_.identifier()));
    buffer >> public_key_;
  }

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

    bls::Signature    sign;
    std::stringstream ssig{static_cast<std::string>(signature)};
    ssig >> sign;

    return sign.verify(public_key_, static_cast<std::string>(data));
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
  Identity       identity_;
  bls::PublicKey public_key_;
};

class BLSSigner : public Prover
{
public:
  BLSSigner()
  {
    details::BLSInitialiser();
  }

  explicit BLSSigner(ConstByteArray const &private_key)
  {
    Load(private_key);
  }

  void Load(ConstByteArray const &private_key) override
  {
    std::stringstream buffer(static_cast<std::string>(private_key));
    buffer >> private_key_;
    private_key_.getPublicKey(public_key_);
  }

  void GenerateKeys()
  {
    private_key_.init();
    private_key_.getPublicKey(public_key_);
  }

  ConstByteArray Sign(ConstByteArray const &text) const final
  {
    std::string    m = static_cast<std::string>(text);
    bls::Signature s;
    private_key_.sign(s, m.c_str());
    std::stringstream signature;
    signature << s;
    return static_cast<ConstByteArray>(signature.str());
  }

  Identity identity() const final
  {
    return Identity(BLS_BN256_UNCOMPRESSED, public_key());
  }

  ConstByteArray public_key() const
  {
    std::stringstream buffer;
    buffer << public_key_;
    return static_cast<ConstByteArray>(buffer.str());
  }

  ConstByteArray private_key()
  {
    std::stringstream buffer;
    buffer << private_key_;
    return static_cast<ConstByteArray>(buffer.str());
  }

private:
  bls::SecretKey private_key_;
  bls::PublicKey public_key_;
};

}  // namespace crypto
}  // namespace fetch
