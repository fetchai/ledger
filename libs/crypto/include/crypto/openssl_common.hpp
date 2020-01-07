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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/openssl_memory.hpp"
#include "crypto/signature_register.hpp"

#include <openssl/obj_mac.h>

#include <cstddef>
#include <stdexcept>

namespace fetch {
namespace crypto {
namespace openssl {

template <int P_ECDSA_Curve_NID = NID_secp256k1>
struct ECDSACurve
{
  static const int         nid;
  static const uint8_t     sn;
  static const std::size_t privateKeySize;
  static const std::size_t publicKeySize;
  static const std::size_t signatureSize;
};

template <int P_ECDSA_Curve_NID>
const int ECDSACurve<P_ECDSA_Curve_NID>::nid = P_ECDSA_Curve_NID;

template <>
const uint8_t ECDSACurve<NID_secp256k1>::sn;
template <>
const std::size_t ECDSACurve<NID_secp256k1>::privateKeySize;
template <>
const std::size_t ECDSACurve<NID_secp256k1>::publicKeySize;
template <>
const std::size_t ECDSACurve<NID_secp256k1>::signatureSize;

using DeleteStrategyType = memory::eDeleteStrategy;

template <typename T, DeleteStrategyType P_DeleteStrategy = DeleteStrategyType::canonical>
using SharedPointerType = memory::OsslSharedPtr<T, P_DeleteStrategy>;

template <typename T, DeleteStrategyType P_DeleteStrategy = DeleteStrategyType::canonical>
using UniquePointerType = memory::ossl_unique_ptr<T, P_DeleteStrategy>;

enum eECDSAEncoding : int
{
  canonical,
  bin,
  DER
};

template <int P_ECDSA_Curve_NID = NID_secp256k1>
class ECDSAAffineCoordinatesConversion
{
public:
  using EcdsaCurveType = ECDSACurve<P_ECDSA_Curve_NID>;
  static const std::size_t x_size;
  static const std::size_t y_size;

  static byte_array::ByteArray Convert2Canonical(BIGNUM const *const x, BIGNUM const *const y)
  {
    auto const xBytes = static_cast<std::size_t>(BN_num_bytes(x));
    auto const yBytes = static_cast<std::size_t>(BN_num_bytes(y));

    byte_array::ByteArray canonical_data;
    canonical_data.Resize(x_size + y_size);

    const std::size_t x_data_start_index = x_size - xBytes;

    for (std::size_t i = 0; i < x_data_start_index; ++i)
    {
      canonical_data[i] = 0;
    }

    if (BN_bn2bin(x, static_cast<uint8_t *>(canonical_data.pointer()) + x_data_start_index) == 0)
    {
      throw std::runtime_error(
          "Convert2Bin<...,"
          "eECDSASignatureBinaryDataFormat::canonical,...>"
          "(): BN_bn2bin(r, ...) "
          "failed.");
    }

    const std::size_t y_data_start_index = x_size + y_size - yBytes;

    for (std::size_t i = x_size; i < y_data_start_index; ++i)
    {
      canonical_data[i] = 0;
    }

    if (BN_bn2bin(y, static_cast<uint8_t *>(canonical_data.pointer()) + y_data_start_index) == 0)
    {
      throw std::runtime_error(
          "Convert2Bin<...,"
          "eECDSASignatureBinaryDataFormat::canonical,...>"
          "(): BN_bn2bin(r, ...) "
          "failed.");
    }

    return canonical_data;
  }

  static void ConvertFromCanonical(byte_array::ConstByteArray const &bin_data, BIGNUM *const x,
                                   BIGNUM *const y)
  {

    if (BN_bin2bn(bin_data.pointer(), static_cast<int>(x_size), x) == nullptr)
    {
      throw std::runtime_error(
          "Convert<...,eECDSASignatureBinaryDataFormat::canonical,...>(const "
          "byte_array::ConstByteArray&): i2d_ECDSA_SIG(..., r) failed.");
    }

    if (BN_bin2bn(bin_data.pointer() + x_size, static_cast<int>(y_size), y) == nullptr)
    {
      throw std::runtime_error(
          "Convert<...,eECDSASignatureBinaryDataFormat::canonical,...>(const "
          "byte_array::ConstByteArray&): i2d_ECDSA_SIG(..., s) failed.");
    }
  }
};

template <int P_ECDSA_Curve_NID>
const std::size_t ECDSAAffineCoordinatesConversion<P_ECDSA_Curve_NID>::x_size =
    ECDSAAffineCoordinatesConversion<P_ECDSA_Curve_NID>::EcdsaCurveType::publicKeySize >> 1u;

template <int P_ECDSA_Curve_NID>
const std::size_t ECDSAAffineCoordinatesConversion<P_ECDSA_Curve_NID>::y_size =
    ECDSAAffineCoordinatesConversion<P_ECDSA_Curve_NID>::x_size;

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
