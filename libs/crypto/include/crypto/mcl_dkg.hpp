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
#include "core/serializers/main_serializer.hpp"
#include "crypto/fetch_mcl.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <set>
#include <sstream>
#include <unordered_map>

namespace bn = mcl::bn256;

namespace fetch {
namespace crypto {
namespace mcl {

namespace details {
struct MCLInitialiser
{
  MCLInitialiser()
  {
    bool a{true};
    a = was_initialised.exchange(a);
    if (!a)
    {
      bn::initPairing();
    }
  }
  static std::atomic<bool> was_initialised;
};
}  // namespace details

/**
 * Classes for BLS Signatures
 */
class PublicKey : public bn::G2
{
public:
  PublicKey();
};

class PrivateKey : public bn::Fr
{
public:
  PrivateKey();
  explicit PrivateKey(uint32_t value);
};

class Signature : public bn::G1
{
public:
  Signature();
};

class Generator : public bn::G2
{
public:
  Generator();
  explicit Generator(std::string const &string_to_hash);
};

struct AggregatePrivateKey
{
  AggregatePrivateKey() = default;
  PrivateKey private_key;
  PrivateKey coefficient;
};

struct AggregatePublicKey
{
  AggregatePublicKey() = default;
  AggregatePublicKey(PublicKey const &public_key, PrivateKey const &coefficient);
  PublicKey aggregate_public_key;
};

struct DkgKeyInformation
{
  DkgKeyInformation() = default;
  DkgKeyInformation(PublicKey group_public_key1, std::vector<PublicKey> public_key_shares1,
                    PrivateKey secret_key_shares1);

  PublicKey              group_public_key;
  std::vector<PublicKey> public_key_shares{};
  PrivateKey             private_key_share;
};

using MessagePayload     = byte_array::ConstByteArray;
using CabinetIndex       = uint32_t;
using SignerRecord       = std::vector<uint8_t>;
using AggregateSignature = std::pair<Signature, SignerRecord>;

/**
 * Vector initialisation for mcl data structures
 *
 * @tparam T Type in vector
 * @param data Vector to be initialised
 * @param i Number of columns
 */
template <typename T>
void Init(std::vector<T> &data, uint32_t i)
{
  data.resize(i);
  for (auto &data_i : data)
  {
    data_i.clear();
  }
}

/**
 * Matrix initialisation for mcl data structures
 *
 * @tparam T Type in matrix
 * @param data Matrix to be initialised
 * @param i Number of rows
 * @param j Number of columns
 */
template <typename T>
void Init(std::vector<std::vector<T>> &data, uint32_t i, uint32_t j)
{
  data.resize(i);
  for (auto &data_i : data)
  {
    data_i.resize(j);
    for (auto &data_ij : data_i)
    {
      data_ij.clear();
    }
  }
}

/**
 * Helper functions for computations used in the DKG
 */
// For DKG
void      SetGenerator(Generator &        generator_g,
                       std::string const &string_to_hash = "Fetch.ai Elliptic Curve Generator G");
void      SetGenerators(Generator &generator_g, Generator &generator_h,
                        std::string const &string_to_hash  = "Fetch.ai Elliptic Curve Generator G",
                        std::string const &string_to_hash2 = "Fetch.ai Elliptic Curve Generator H");
PublicKey ComputeLHS(PublicKey &tmpG, Generator const &G, Generator const &H,
                     PrivateKey const &share1, PrivateKey const &share2);
PublicKey ComputeLHS(Generator const &G, Generator const &H, PrivateKey const &share1,
                     PrivateKey const &share2);
void      UpdateRHS(uint32_t rank, PublicKey &rhsG, std::vector<PublicKey> const &input);
PublicKey ComputeRHS(uint32_t rank, std::vector<PublicKey> const &input);
void      ComputeShares(PrivateKey &s_i, PrivateKey &sprime_i, std::vector<PrivateKey> const &a_i,
                        std::vector<PrivateKey> const &b_i, uint32_t index);
std::vector<PrivateKey> InterpolatePolynom(std::vector<PrivateKey> const &a,
                                           std::vector<PrivateKey> const &b);

// For signatures
Signature SignShare(MessagePayload const &message, PrivateKey const &x_i);
bool      VerifySign(PublicKey const &y, MessagePayload const &message, Signature const &sign,
                     Generator const &G);
Signature LagrangeInterpolation(std::unordered_map<CabinetIndex, Signature> const &shares);
std::vector<DkgKeyInformation> TrustedDealerGenerateKeys(uint32_t cabinet_size, uint32_t threshold);
std::pair<PrivateKey, PublicKey> GenerateKeyPair(Generator const &generator);

// For aggregate signatures. Note only the verification of the signatures is done using VerifySign
// but one must compute the public key to verify with
PrivateKey         SignatureAggregationCoefficient(PublicKey const &             notarisation_key,
                                                   std::vector<PublicKey> const &cabinet_notarisation_keys);
Signature          AggregateSign(MessagePayload const &     message,
                                 AggregatePrivateKey const &aggregate_private_key);
AggregateSignature ComputeAggregateSignature(
    std::unordered_map<uint32_t, Signature> const &signatures, uint32_t cabinet_size);
PublicKey ComputeAggregatePublicKey(SignerRecord const &          signers,
                                    std::vector<PublicKey> const &cabinet_public_keys);
PublicKey ComputeAggregatePublicKey(SignerRecord const &                   signers,
                                    std::vector<AggregatePublicKey> const &cabinet_public_keys);

}  // namespace mcl
}  // namespace crypto

namespace serializers {
template <typename D>
struct ArraySerializer<crypto::mcl::Signature, D>
{

public:
  using Type       = crypto::mcl::Signature;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &b)
  {
    auto array = array_constructor(1);
    array.Append(b.getStr());
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &b)
  {
    std::string sig_str;
    array.GetNextValue(sig_str);
    bool check;
    b.setStr(&check, sig_str.data());
    if (!check)
    {
      throw SerializableException(error::TYPE_ERROR,
                                  std::string("String does not convert to MCL type"));
    }
  }
};

template <typename D>
struct ArraySerializer<crypto::mcl::PrivateKey, D>
{

public:
  using Type       = crypto::mcl::PrivateKey;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &b)
  {
    auto array = array_constructor(1);
    array.Append(b.getStr());
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &b)
  {
    std::string sig_str;
    array.GetNextValue(sig_str);
    bool check;
    b.setStr(&check, sig_str.data());
    if (!check)
    {
      throw SerializableException(error::TYPE_ERROR,
                                  std::string("String does not convert to MCL type"));
    }
  }
};

template <typename D>
struct ArraySerializer<crypto::mcl::PublicKey, D>
{

public:
  using Type       = crypto::mcl::PublicKey;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &b)
  {
    auto array = array_constructor(1);
    array.Append(b.getStr());
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &b)
  {
    std::string sig_str;
    array.GetNextValue(sig_str);
    bool check;
    b.setStr(&check, sig_str.data());
    if (!check)
    {
      throw SerializableException(error::TYPE_ERROR,
                                  std::string("String does not convert to MCL type"));
    }
  }
};

template <typename V, typename D>
struct ArraySerializer<std::pair<crypto::mcl::PublicKey, V>, D>
{
public:
  using Type       = std::pair<crypto::mcl::PublicKey, V>;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &array_constructor, Type const &input)
  {
    auto array = array_constructor(2);
    array.Append(input.first.getStr());
    array.Append(input.second);
  }

  template <typename ArrayDeserializer>
  static void Deserialize(ArrayDeserializer &array, Type &output)
  {
    if (array.size() != 2)
    {
      throw SerializableException(std::string("std::pair must have exactly 2 elements."));
    }

    std::string key_str;
    array.GetNextValue(key_str);
    output.first.clear();
    bool check;
    output.first.setStr(&check, key_str.data());
    if (!check)
    {
      throw SerializableException(error::TYPE_ERROR,
                                  std::string("String does not convert to MCL type"));
    }
    array.GetNextValue(output.second);
  }
};
}  // namespace serializers
}  // namespace fetch
