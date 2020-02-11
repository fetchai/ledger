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

#include <utility>

#include "crypto/openssl_common.hpp"
#include "crypto/openssl_context_session.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template <eECDSAEncoding          P_ECDSABinaryDataFormat = eECDSAEncoding::canonical,
          int                     P_ECDSA_Curve_NID       = NID_secp256k1,
          point_conversion_form_t P_ConversionForm        = POINT_CONVERSION_UNCOMPRESSED>
class ECDSAPublicKey
{
  SharedPointerType<EC_POINT> key_EC_POINT_;
  SharedPointerType<EC_KEY>   key_EC_KEY_;
  byte_array::ConstByteArray  key_binary_;

public:
  using EcdsaCurveType = ECDSACurve<P_ECDSA_Curve_NID>;

  static constexpr eECDSAEncoding          binaryDataFormat = P_ECDSABinaryDataFormat;
  static constexpr point_conversion_form_t conversionForm   = P_ConversionForm;

  template <eECDSAEncoding P_ECDSABinaryDataFormat2, int P_ECDSA_Curve_NID2,
            point_conversion_form_t P_ConversionForm2>
  friend class ECDSAPublicKey;

  ECDSAPublicKey() = default;

  ECDSAPublicKey(SharedPointerType<EC_POINT> &&public_key, const EC_GROUP *group,
                 const context::Session<BN_CTX> &session)
    : key_EC_POINT_{public_key}
    , key_EC_KEY_{ConvertToECKEY(public_key.get())}
    , key_binary_{Convert(public_key.get(), group, session, binaryDataFormat)}
  {}

  explicit ECDSAPublicKey(byte_array::ConstByteArray key_data)
    : key_EC_POINT_{Convert(key_data, binaryDataFormat)}
    , key_EC_KEY_{ConvertToECKEY(key_EC_POINT_.get())}
    , key_binary_{std::move(key_data)}
  {}

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  using EcdsaPublicKeyType =
      ECDSAPublicKey<BINARY_DATA_FORMAT, P_ECDSA_Curve_NID, P_ConversionForm>;

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  explicit ECDSAPublicKey(EcdsaPublicKeyType<BINARY_DATA_FORMAT> const &from)
    : key_EC_POINT_{from.key_EC_POINT_}
    , key_EC_KEY_{from.key_EC_KEY_}
    , key_binary_{BINARY_DATA_FORMAT == binaryDataFormat
                      ? from.key_binary_
                      : Convert(key_EC_POINT_.get(), binaryDataFormat)}
  {}

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  explicit ECDSAPublicKey(EcdsaPublicKeyType<BINARY_DATA_FORMAT> &&from)
    : key_EC_POINT_{std::move(from.key_EC_POINT_)}
    , key_EC_KEY_{std::move(from.key_EC_KEY_)}
    , key_binary_{BINARY_DATA_FORMAT == binaryDataFormat
                      ? std::move(from.key_binary_)
                      : Convert(key_EC_POINT_.get(), binaryDataFormat)}
  {}

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  ECDSAPublicKey &operator=(EcdsaPublicKeyType<BINARY_DATA_FORMAT> const &from)
  {
    *this = ECDSAPublicKey(from);
    return *this;
  }

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  ECDSAPublicKey &operator=(EcdsaPublicKeyType<BINARY_DATA_FORMAT> &&from)
  {
    *this = ECDSAPublicKey(from);
    return *this;
  }

  SharedPointerType<const EC_POINT> KeyAsECPoint() const
  {
    return key_EC_POINT_;
  }

  SharedPointerType<const EC_KEY> key() const
  {
    return key_EC_KEY_;
  }

  byte_array::ConstByteArray const &KeyAsBin() const
  {
    return key_binary_;
  }

private:
  using AffineCoordConversionType = ECDSAAffineCoordinatesConversion<P_ECDSA_Curve_NID>;

  static byte_array::ByteArray Convert(EC_POINT const *const           public_key,
                                       EC_GROUP const *const           group,
                                       context::Session<BN_CTX> const &session,
                                       eECDSAEncoding const            binary_data_format)
  {
    switch (binary_data_format)
    {
    case eECDSAEncoding::canonical:
      return Convert2Canonical(public_key, group, session);

    case eECDSAEncoding::bin:
      return Convert2Bin(public_key, group, session);

    case eECDSAEncoding::DER:
      throw std::domain_error(
          "ECDSAPublicKey::Convert(...): Conversion in to "
          "DER encoded data is NOT implemented "
          "yet.");
    }

    return {};
  }

  static byte_array::ByteArray Convert(EC_POINT const *const public_key,
                                       eECDSAEncoding const  binary_data_format)
  {
    UniquePointerType<EC_GROUP> group{createGroup()};
    context::Session<BN_CTX>    session;

    switch (binary_data_format)
    {
    case eECDSAEncoding::canonical:
    case eECDSAEncoding::bin:
      return Convert(public_key, group.get(), session, binaryDataFormat);

    case eECDSAEncoding::DER:
      return Convert(public_key, group.get(), session, binaryDataFormat);
    }

    return {};
  }

  static UniquePointerType<EC_POINT> Convert(byte_array::ConstByteArray const &key_data,
                                             eECDSAEncoding const              binary_data_format)
  {
    switch (binary_data_format)
    {
    case eECDSAEncoding::canonical:
      return ConvertFromCanonical(key_data);

    case eECDSAEncoding::bin:
      return ConvertFromBin(key_data);

    case eECDSAEncoding::DER:
      throw std::domain_error(
          "ECDSAPublicKey::Convert(...): Conversion from "
          "DER encoded data is NOT implemented yet.");
    }

    return {};
  }

  static byte_array::ByteArray Convert2Canonical(EC_POINT const *const           public_key,
                                                 EC_GROUP const *const           group,
                                                 context::Session<BN_CTX> const &session)
  {
    SharedPointerType<BIGNUM> x{BN_new()};
    SharedPointerType<BIGNUM> y{BN_new()};
    EC_POINT_get_affine_coordinates_GFp(group, public_key, x.get(), y.get(),
                                        session.context().get());
    return AffineCoordConversionType::Convert2Canonical(x.get(), y.get());
  }

  static byte_array::ByteArray Convert2Bin(EC_POINT const *const           public_key,
                                           EC_GROUP const *const           group,
                                           context::Session<BN_CTX> const &session)
  {
    SharedPointerType<BIGNUM> public_key_as_BN{BN_new()};
    if (EC_POINT_point2bn(group, public_key, ECDSAPublicKey::conversionForm, public_key_as_BN.get(),
                          session.context().get()) == nullptr)
    {
      throw std::runtime_error(
          "ECDSAPublicKey::Convert(...): "
          "`EC_POINT_point2bn(...)` functioni failed.");
    }

    byte_array::ByteArray pub_key_as_bin;
    pub_key_as_bin.Resize(static_cast<std::size_t>(BN_num_bytes(public_key_as_BN.get())));

    if (BN_bn2bin(public_key_as_BN.get(), static_cast<uint8_t *>(pub_key_as_bin.pointer())) == 0)
    {
      throw std::runtime_error("ECDSAPublicKey::Convert(...): `BN_bn2bin(...)` function failed.");
    }

    return pub_key_as_bin;
  }

  static UniquePointerType<EC_POINT> ConvertFromCanonical(
      byte_array::ConstByteArray const &key_data)
  {
    UniquePointerType<EC_GROUP> group{createGroup()};
    UniquePointerType<EC_POINT> public_key{EC_POINT_new(group.get())};
    context::Session<BN_CTX>    session;

    SharedPointerType<BIGNUM> x{BN_new()};
    SharedPointerType<BIGNUM> y{BN_new()};

    AffineCoordConversionType::ConvertFromCanonical(key_data, x.get(), y.get());

    if (EC_POINT_set_affine_coordinates_GFp(group.get(), public_key.get(), x.get(), y.get(),
                                            session.context().get()) == 0)
    {
      throw std::runtime_error(
          "ECDSAPublicKey::ConvertFromCanonical(...): "
          "`EC_POINT_set_affine_coordinates_GFp(...)` function failed.");
    }

    return public_key;
  }

  static UniquePointerType<EC_POINT> ConvertFromBin(byte_array::ConstByteArray const &key_data)
  {
    SharedPointerType<BIGNUM> pub_key_as_BN{BN_new()};
    if (BN_bin2bn(static_cast<const uint8_t *>(key_data.pointer()), int(key_data.size()),
                  pub_key_as_BN.get()) == nullptr)
    {
      throw std::runtime_error("ECDSAPublicKey::ConvertToECPOINT(...): BN_bin2bn(...) failed.");
    }

    UniquePointerType<EC_GROUP> group{createGroup()};
    UniquePointerType<EC_POINT> public_key{EC_POINT_new(group.get())};
    context::Session<BN_CTX>    session;

    if (EC_POINT_bn2point(group.get(), pub_key_as_BN.get(), public_key.get(),
                          session.context().get()) == nullptr)
    {
      throw std::runtime_error(
          "ECDSAPublicKey::ConvertToECPOINT(...): "
          "EC_POINT_bn2point(...) failed.");
    }

    return public_key;
  }

  static UniquePointerType<EC_KEY> ConvertToECKEY(EC_POINT const *key_EC_POINT)
  {
    UniquePointerType<EC_KEY> key{EC_KEY_new_by_curve_name(EcdsaCurveType::nid)};
    // TODO(issue 36): setting conv. form might not be really necessary (stuff
    // works
    // without it)
    EC_KEY_set_conv_form(key.get(), conversionForm);

    if (EC_KEY_set_public_key(key.get(), key_EC_POINT) == 0)
    {
      throw std::runtime_error(
          "ECDSAPublicKey::ConvertToECKEY(...): "
          "EC_KEY_set_public_key(...) failed.");
    }

    return key;
  }

  static UniquePointerType<EC_GROUP> createGroup()
  {
    UniquePointerType<EC_GROUP> group{EC_GROUP_new_by_curve_name(EcdsaCurveType::nid)};
    EC_GROUP_set_point_conversion_form(group.get(), conversionForm);
    return group;
  }
};

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
