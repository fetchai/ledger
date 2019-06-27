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
#include <bls/bls.h>

namespace fetch {
namespace crypto {

namespace bls {
using PrivateKey = blsSecretKey;
using PublicKey  = blsPublicKey;
using Id         = blsId;
using Signature  = blsSignature;

using PublicKeyList  = std::vector<PublicKey>;
using PrivateKeyList = std::vector<PrivateKey>;
using IdList         = std::vector<Id>;
using SignatureList  = std::vector<Signature>;

enum
{
  E_MCLBN_CURVE_FP254BNB = 0,
  E_MCLBN_CURVE_FP382_1  = 1,
  E_MCLBN_CURVE_FP382_2  = 2,
  E_MCL_BLS12_381        = 5,
  E_MCLBN_FP_UNIT_SIZE   = 6,
  E_FR_SIZE              = E_MCLBN_FP_UNIT_SIZE * 8,
  E_ID_SIZE              = E_FR_SIZE,
  E_G1_SIZE              = E_FR_SIZE * 3,
  E_G2_SIZE              = E_FR_SIZE * 3 * 2
};

void Init()
{

  // TODO: add atomic lock.
  if (blsInit(E_MCLBN_CURVE_FP254BNB, MCLBN_COMPILED_TIME_VAR) != 0)
  {
    throw std::runtime_error("unable to initalize BLS.");
  }
}

PrivateKey PrivateKeyByCSPRNG()
{
  PrivateKey ret;

  int32_t error = blsSecretKeySetByCSPRNG(&ret);

  if (error != 0)
  {
    throw std::runtime_error("Failed at generating BLS secret key.");
  }

  return ret;
}

PublicKey PublicKeyFromPrivate(PrivateKey const &priv)
{
  PublicKey ret;
  blsGetPublicKey(&ret, &priv);
  return ret;
}

Signature Sign(PrivateKey const &priv, byte_array::ConstByteArray const &msg)
{
  Signature ret;
  blsSign(&ret, &priv, msg.pointer(), msg.size());
  return ret;
}

bool Verify(Signature const &signature, PublicKey const &pub, byte_array::ConstByteArray const &msg)
{
  return blsVerify(&signature, &pub, msg.pointer(), msg.size());
}

PrivateKey HashToPrivateKey(byte_array::ConstByteArray const &seed)
{
  PrivateKey priv;
  blsHashToSecretKey(&priv, seed.pointer(), seed.size());
  return priv;
}

template <typename KeyType>
KeyType PrivateKeyShare(std::vector<KeyType> &kl, Id id)
{
  KeyType ret;
  int32_t error = blsSecretKeyShare(&ret, kl.data(), kl.size(), &id);

  if (error != 0)
  {
    throw std::runtime_error("failed to generate private key share");
  }

  return ret;
}

void AddKeys(PrivateKey &lhs, PrivateKey const &rhs)
{
  blsSecretKeyAdd(&lhs, &rhs);
}

void AddKeys(PublicKey &lhs, PublicKey const &rhs)
{
  blsPublicKeyAdd(&lhs, &rhs);
}

bool PublicKeyIsEqual(PublicKey const &pk1, PublicKey const &pk2)
{
  return blsPublicKeyIsEqual(&pk1, &pk2);
}

PublicKey GetPublicKey(PrivateKey const &sk)
{
  PublicKey ret;
  blsGetPublicKey(&ret, &sk);
  return ret;
}

PublicKey PublicKeyShare(PublicKeyList const &master_keys, Id const &id)
{
  PublicKey ret;
  blsPublicKeyShare(&ret, master_keys.data(), master_keys.size(), &id);
  return ret;
}

Signature RecoverSignature(SignatureList const &sigs, IdList const &ids)
{
  Signature ret;
  if (blsSignatureRecover(&ret, sigs.data(), ids.data(), sigs.size()) != 0)
  {
    throw std::runtime_error("Unable to recover signature");
  }
  return ret;
}
};  // namespace bls

}  // namespace crypto
}  // namespace fetch
