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

#include "crypto/openssl_ecdsa_public_key.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template <eECDSAEncoding P_ECDSABinaryDataFormat>
struct SupportedEncodingForPublicKey
{
  static constexpr eECDSAEncoding value = P_ECDSABinaryDataFormat;
};

template <>
struct SupportedEncodingForPublicKey<eECDSAEncoding::DER>
{
  static constexpr eECDSAEncoding value = eECDSAEncoding::bin;
};

template <eECDSAEncoding          P_ECDSABinaryDataFormat = eECDSAEncoding::canonical,
          int                     P_ECDSA_Curve_NID       = NID_secp256k1,
          point_conversion_form_t P_ConversionForm        = POINT_CONVERSION_UNCOMPRESSED>
class ECDSAPrivateKey
{
public:
  static constexpr eECDSAEncoding          binaryDataFormat = P_ECDSABinaryDataFormat;
  static constexpr point_conversion_form_t conversionForm   = P_ConversionForm;

  // using public_key_type = ECDSAPublicKey<binaryDataFormat, P_ECDSA_Curve_NID,
  // P_ConversionForm>;
  // TODO(issue 36): Implement DER encoding. It mis missing now so defaulting to
  // canonical
  // encoding to void
  // failures when constructing this class (ECDSAPrivateKey) with DER encoding.
  using public_key_type =
      ECDSAPublicKey<SupportedEncodingForPublicKey<P_ECDSABinaryDataFormat>::value,
                     P_ECDSA_Curve_NID, P_ConversionForm>;

  using ecdsa_curve_type = ECDSACurve<P_ECDSA_Curve_NID>;

  template <eECDSAEncoding P_ECDSABinaryDataFormat2, int P_ECDSA_Curve_NID2,
            point_conversion_form_t P_ConversionForm2>
  friend class ECDSAPrivateKey;

private:
  // TODO(issue 36): Keep key encrypted
  shrd_ptr_type<EC_KEY> private_key_;
  // TODO(issue 36): Do lazy initialisation of the public key to minimize impact
  // at construction time of this class
  public_key_type public_key_;

public:
  ECDSAPrivateKey()
    : ECDSAPrivateKey(Generate())
  {}

  ECDSAPrivateKey(const byte_array::ConstByteArray &key_data)
    : ECDSAPrivateKey(Convert(key_data))
  {}

  template <eECDSAEncoding P_ECDSABinaryDataFormat2, int P_ECDSA_Curve_NID2,
            point_conversion_form_t P_ConversionForm2>
  friend class ECDSAPrivateKey;

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  using private_key_type = ECDSAPrivateKey<BINARY_DATA_FORMAT, P_ECDSA_Curve_NID, P_ConversionForm>;

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  ECDSAPrivateKey(private_key_type<BINARY_DATA_FORMAT> const &from)
    : private_key_(from.private_key_)
    , public_key_(from.public_key_)
  {}

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  ECDSAPrivateKey(private_key_type<BINARY_DATA_FORMAT> &&from)
    : private_key_(std::move(from.private_key_))
    , public_key_(std::move(from.public_key_))
  {}

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  ECDSAPrivateKey &operator=(private_key_type<BINARY_DATA_FORMAT> const &from)
  {
    private_key_ = from.private_key_;
    public_key_  = from.public_key_;
    return *this;
  }

  template <eECDSAEncoding BINARY_DATA_FORMAT>
  ECDSAPrivateKey &operator=(private_key_type<BINARY_DATA_FORMAT> &&from)
  {
    private_key_ = std::move(from.private_key_);
    public_key_  = std::move(from.public_key_);
    return *this;
  }

  shrd_ptr_type<const EC_KEY> key() const
  {
    return private_key_;
  }

  byte_array::ByteArray KeyAsBin() const
  {
    switch (binaryDataFormat)
    {
    case eECDSAEncoding::canonical:
    case eECDSAEncoding::bin:
      return Convert2Bin(private_key_.get());

    case eECDSAEncoding::DER:
      return Convert2DER(private_key_.get());
    }
  }

  const public_key_type &publicKey() const
  {
    return public_key_;
  }

private:
  ECDSAPrivateKey(shrd_ptr_type<EC_KEY> &&key, public_key_type &&public_key)
    : private_key_{std::move(key)}
    , public_key_{std::move(public_key)}
  {}

  static ECDSAPrivateKey Convert(byte_array::ConstByteArray const &key_data)
  {
    switch (binaryDataFormat)
    {
    case eECDSAEncoding::canonical:
    case eECDSAEncoding::bin:
      return ConvertFromBin(key_data);

    case eECDSAEncoding::DER:
      return ConvertFromDER(key_data);
    }
  }

  static uniq_ptr_type<BIGNUM, del_strat_type::clearing> Convert2BIGNUM(
      byte_array::ConstByteArray const &key_data)
  {
    if (ecdsa_curve_type::privateKeySize < key_data.size())
    {
      throw std::runtime_error(
          "ECDSAPrivateKey::Convert2BIGNUM(const "
          "byte_array::ConstByteArray&): Length of "
          "provided "
          "byte array does not correspond to expected "
          "length for selected elliptic curve");
    }

    uniq_ptr_type<BIGNUM, del_strat_type::clearing> private_key_as_BN(BN_new());
    BN_bin2bn(key_data.pointer(), static_cast<int>(ecdsa_curve_type::privateKeySize),
              private_key_as_BN.get());

    if (!private_key_as_BN)
    {
      throw std::runtime_error(
          "ECDSAPrivateKey::Convert2BIGNUM(const "
          "byte_array::ConstByteArray&): BN_bin2bn(...) "
          "failed.");
    }

    return private_key_as_BN;
  }

  static uniq_ptr_type<EC_KEY> ConvertPrivateKeyBN2ECKEY(BIGNUM const *private_key_as_BN)
  {
    uniq_ptr_type<EC_KEY> private_key{EC_KEY_new_by_curve_name(ecdsa_curve_type::nid)};
    EC_KEY_set_conv_form(private_key.get(), conversionForm);

    if (!EC_KEY_set_private_key(private_key.get(), private_key_as_BN))
    {
      throw std::runtime_error(
          "ECDSAPrivateKey::ConvertPrivateKeyBN2ECKEY(...)"
          ": EC_KEY_set_private_key(...) failed.");
    }

    return private_key;
  }

  static public_key_type DerivePublicKey(BIGNUM const *const private_key_as_BN,
                                         EC_KEY *const       private_key,
                                         bool const regenerate_even_if_already_exists = false)
  {
    EC_GROUP const *const    group = EC_KEY_get0_group(private_key);
    context::Session<BN_CTX> session;

    uniq_ptr_type<EC_POINT> public_key;
    if (!regenerate_even_if_already_exists)
    {
      EC_POINT const *const pub_key_ptr_ref = EC_KEY_get0_public_key(private_key);
      public_key.reset(EC_POINT_dup(pub_key_ptr_ref, group));
    }

    if (!public_key)
    {
      public_key.reset(EC_POINT_new(group));
    }

    if (!public_key)
    {
      throw std::runtime_error(
          "ECDSAPrivateKey::DerivePublicKey(...): "
          "EC_POINT_(new/dup)(...) failed.");
    }

    if (!EC_POINT_mul(group, public_key.get(), private_key_as_BN, NULL, NULL,
                      session.context().get()))
    {
      throw std::runtime_error("ECDSAPrivateKey::DerivePublicKey(...): EC_POINT_mul(...) failed.");
    }

    //* The `EC_KEY_set_public_key(...)` creates it's own duplicate of the
    // EC_POINT
    if (!EC_KEY_set_public_key(private_key, public_key.get()))
    {
      throw std::runtime_error(
          "ECDSAPrivateKey::DerivePublicKey(...): "
          "EC_KEY_set_public_key(...) failed.");
    }

    return public_key_type{std::move(public_key), group, session};
  }

  static ECDSAPrivateKey Generate()
  {
    uniq_ptr_type<EC_KEY> key{GenerateKeyPair()};
    public_key_type       public_key{ExtractPublicKey(key.get())};
    return ECDSAPrivateKey{std::move(key), std::move(public_key)};
  }

  static public_key_type ExtractPublicKey(const EC_KEY *private_key)
  {
    EC_GROUP const *group             = EC_KEY_get0_group(private_key);
    EC_POINT const *pub_key_reference = EC_KEY_get0_public_key(private_key);

    uniq_ptr_type<EC_POINT> public_key{EC_POINT_dup(pub_key_reference, group)};
    if (!public_key)
    {
      throw std::runtime_error("ECDSAPrivateKey::ExtractPublicKey(...): EC_POINT_dup(...) failed.");
    }

    return public_key_type{std::move(public_key), group, context::Session<BN_CTX>{}};
  }

  static uniq_ptr_type<EC_KEY> GenerateKeyPair()
  {
    uniq_ptr_type<EC_KEY> key_pair{EC_KEY_new_by_curve_name(ecdsa_curve_type::nid)};
    EC_KEY_set_conv_form(key_pair.get(), conversionForm);

    if (!key_pair)
    {
      throw std::runtime_error(
          "ECDSAPrivateKey::GenerateKeyPair(): "
          "EC_KEY_new_by_curve_name(...) failed.");
    }

    if (!EC_KEY_generate_key(key_pair.get()))
    {
      throw std::runtime_error(
          "ECDSAPrivateKey::GenerateKeyPair(): "
          "EC_KEY_generate_key(...) failed.");
    }

    return key_pair;
  }

  static byte_array::ByteArray Convert2Bin(EC_KEY const *const key)
  {
    const BIGNUM *key_as_BN = EC_KEY_get0_private_key(key);
    if (!key_as_BN)
    {
      throw std::runtime_error("ECDSAPrivateKey::keyAsBin(): EC_KEY_get0_private_key(...) failed.");
    }

    byte_array::ByteArray key_as_bin;
    key_as_bin.Resize(static_cast<std::size_t>(BN_num_bytes(key_as_BN)));

    if (!BN_bn2bin(key_as_BN, static_cast<unsigned char *>(key_as_bin.pointer())))
    {
      throw std::runtime_error("ECDSAPrivateKey::keyAsBin(...): BN_bn2bin(...) failed.");
    }

    return key_as_bin;
  }

  static byte_array::ByteArray Convert2DER(EC_KEY *key)
  {
    const int est_size = i2d_ECPrivateKey(key, nullptr);
    if (est_size < 1)
    {
      throw std::runtime_error(
          "ECDSAPrivateKey::Convert2DER(...): "
          "i2d_ECPrivateKey(..., nullptr) failed.");
    }

    byte_array::ByteArray key_as_bin;
    key_as_bin.Resize(static_cast<std::size_t>(est_size));

    unsigned char *key_as_bin_ptr = static_cast<unsigned char *>(key_as_bin.pointer());
    const int      res_size       = i2d_ECPrivateKey(key, &key_as_bin_ptr);
    if (res_size < 1 || res_size > est_size)
    {
      throw std::runtime_error("ECDSAPrivateKey::Convert2DER(...): i2d_ECPrivateKey(...) failed.");
    }

    key_as_bin.Resize(static_cast<std::size_t>(res_size));

    return key_as_bin;
  }

  static ECDSAPrivateKey ConvertFromBin(byte_array::ConstByteArray const &key_data)
  {
    uniq_ptr_type<BIGNUM, del_strat_type::clearing> priv_key_as_BN{Convert2BIGNUM(key_data)};

    uniq_ptr_type<EC_KEY> private_key{ConvertPrivateKeyBN2ECKEY(priv_key_as_BN.get())};
    public_key_type       public_key{DerivePublicKey(priv_key_as_BN.get(), private_key.get())};

    return ECDSAPrivateKey{std::move(private_key), std::move(public_key)};
  }

  static ECDSAPrivateKey ConvertFromDER(byte_array::ConstByteArray const &key_data)
  {
    uniq_ptr_type<EC_KEY> key{EC_KEY_new_by_curve_name(ecdsa_curve_type::nid)};
    EC_KEY_set_conv_form(key.get(), conversionForm);

    EC_KEY *             key_ptr      = key.get();
    const unsigned char *key_data_ptr = static_cast<const unsigned char *>(key_data.pointer());
    if (!d2i_ECPrivateKey(&key_ptr, &key_data_ptr, static_cast<long>(key_data.size())))
    {
      throw std::runtime_error(
          "ECDSAPrivateKey::ConvertFromDER(...): "
          "d2i_ECPrivateKey(...) failed.");
    }

    BIGNUM const *const private_key_as_BN = EC_KEY_get0_private_key(key_ptr);

    public_key_type public_key{DerivePublicKey(private_key_as_BN, key_ptr)};

    return ECDSAPrivateKey{std::move(key), std::move(public_key)};
  }

  static uniq_ptr_type<EC_GROUP> createGroup()
  {
    uniq_ptr_type<EC_GROUP> group{EC_GROUP_new_by_curve_name(ecdsa_curve_type::nid)};
    EC_GROUP_set_point_conversion_form(group.get(), conversionForm);
    return group;
  }
};

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
