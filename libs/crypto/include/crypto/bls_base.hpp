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

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/group_definitions.hpp"

#include "bls/bls.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

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

void Init();

inline PrivateKey PrivateKeyByCSPRNG()
{
  PrivateKey ret;

  int32_t error = blsSecretKeySetByCSPRNG(&ret);

  if (error != 0)
  {
    throw std::runtime_error("Failed at generating BLS secret key.");
  }

  return ret;
}

inline PublicKey PublicKeyFromPrivate(PrivateKey const &priv)
{
  PublicKey ret;
  blsGetPublicKey(&ret, &priv);
  return ret;
}

inline Signature Sign(PrivateKey const &priv, byte_array::ConstByteArray const &msg)
{
  Signature ret;
  blsSign(&ret, &priv, msg.pointer(), msg.size());
  return ret;
}

inline bool Verify(Signature const &signature, PublicKey const &pub,
                   byte_array::ConstByteArray const &msg)
{
  return blsVerify(&signature, &pub, msg.pointer(), msg.size()) != 0;
}

inline PrivateKey HashToPrivateKey(byte_array::ConstByteArray const &seed)
{
  PrivateKey priv;
  blsHashToSecretKey(&priv, seed.pointer(), seed.size());
  return priv;
}

template <typename KeyType>
KeyType PrivateKeyShare(std::vector<KeyType> &kl, Id const &id)
{
  KeyType ret{};
  int32_t error = blsSecretKeyShare(&ret, kl.data(), kl.size(), &id);

  if (error != 0)
  {
    throw std::runtime_error("failed to generate private key share");
  }

  return ret;
}

inline void AddKeys(PrivateKey &lhs, PrivateKey const &rhs)
{
  blsSecretKeyAdd(&lhs, &rhs);
}

inline void AddKeys(PublicKey &lhs, PublicKey const &rhs)
{
  blsPublicKeyAdd(&lhs, &rhs);
}

inline bool PublicKeyIsEqual(PublicKey const &pk1, PublicKey const &pk2)
{
  return blsPublicKeyIsEqual(&pk1, &pk2) != 0;
}

inline PublicKey GetPublicKey(PrivateKey const &sk)
{
  PublicKey ret;
  blsGetPublicKey(&ret, &sk);
  return ret;
}

inline PublicKey PublicKeyShare(PublicKeyList const &master_keys, Id const &id)
{
  PublicKey ret;
  blsPublicKeyShare(&ret, master_keys.data(), master_keys.size(), &id);
  return ret;
}

inline Signature RecoverSignature(SignatureList const &sigs, IdList const &ids)
{
  Signature ret;
  if (blsSignatureRecover(&ret, sigs.data(), ids.data(), sigs.size()) != 0)
  {
    throw std::runtime_error("Unable to recover signature");
  }
  return ret;
}

inline byte_array::ConstByteArray ToBinary(Signature const &sig)
{
  byte_array::ByteArray buffer{};
  buffer.Resize(1024);
#ifdef BLS_SWAP_G
  std::size_t n = mclBnG2_getStr(buffer.char_pointer(), buffer.size(), &sig.v, 0);
#else
  std::size_t n = mclBnG1_getStr(buffer.char_pointer(), buffer.size(), &sig.v, 0);
#endif
  if (n == 0)
  {
    throw std::runtime_error("Signature:tgetStr");
  }
  buffer.Resize(n);

  return {buffer};
}

}  // namespace bls
}  // namespace crypto

namespace serializers {

template <typename D>
struct BinarySerializer<::blsId, D>
{
public:
  using Type       = ::blsId;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &binary_construtor, Type const &id)
  {
    auto        data = binary_construtor(sizeof(id));
    auto const *raw  = reinterpret_cast<uint8_t const *>(&id);
    data.Write(raw, sizeof(id));
  }

  template <typename BinaryDeserializer>
  static void Deserialize(BinaryDeserializer &binary_stream, Type &id)
  {
    auto *raw = reinterpret_cast<uint8_t *>(&id);
    binary_stream.Read(raw, sizeof(id));
  }
};

template <typename D>
struct BinarySerializer<::blsPublicKey, D>
{
public:
  using Type       = ::blsPublicKey;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &binary_construtor, Type const &id)
  {
    auto        data = binary_construtor(sizeof(id));
    auto const *raw  = reinterpret_cast<uint8_t const *>(&id);
    data.Write(raw, sizeof(id));
  }

  template <typename BinaryDeserializer>
  static void Deserialize(BinaryDeserializer &binary_stream, Type &id)
  {
    auto *raw = reinterpret_cast<uint8_t *>(&id);
    binary_stream.Read(raw, sizeof(id));
  }
};

template <typename D>
struct BinarySerializer<::blsSecretKey, D>
{
public:
  using Type       = ::blsSecretKey;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &binary_construtor, Type const &id)
  {
    auto        data = binary_construtor(sizeof(id));
    auto const *raw  = reinterpret_cast<uint8_t const *>(&id);
    data.Write(raw, sizeof(id));
  }

  template <typename BinaryDeserializer>
  static void Deserialize(BinaryDeserializer &binary_stream, Type &id)
  {
    auto *raw = reinterpret_cast<uint8_t *>(&id);
    binary_stream.Read(raw, sizeof(id));
  }
};

template <typename D>
struct BinarySerializer<::blsSignature, D>
{
public:
  using Type       = ::blsSignature;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &binary_construtor, Type const &id)
  {
    auto        data = binary_construtor(sizeof(id));
    auto const *raw  = reinterpret_cast<uint8_t const *>(&id);
    data.Write(raw, sizeof(id));
  }

  template <typename BinaryDeserializer>
  static void Deserialize(BinaryDeserializer &binary_stream, Type &id)
  {
    auto *raw = reinterpret_cast<uint8_t *>(&id);
    binary_stream.Read(raw, sizeof(id));
  }
};

}  // namespace serializers
}  // namespace fetch
