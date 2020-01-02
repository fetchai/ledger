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

#include "core/byte_array/encoders.hpp"
#include "crypto/hash.hpp"
#include "crypto/openssl_ecdsa_private_key.hpp"
#include "crypto/sha256.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace fetch {
namespace crypto {
namespace openssl {

template <eECDSAEncoding P_ECDSASignatureBinaryDataFormat = eECDSAEncoding::canonical,
          typename T_Hasher = SHA256, int P_ECDSA_Curve_NID = NID_secp256k1>
class ECDSASignature
{
  byte_array::ConstByteArray   hash_;
  SharedPointerType<ECDSA_SIG> signature_ecdsa_ptr_;
  byte_array::ConstByteArray   signature_;

public:
  using HasherType     = T_Hasher;
  using EcdsaCurveType = ECDSACurve<P_ECDSA_Curve_NID>;
  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  using PublicKeyType = ECDSAPublicKey<BIN_ENC, P_ECDSA_Curve_NID, POINT_CONV_FORM>;
  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  using PrivateKeyType = ECDSAPrivateKey<BIN_ENC, P_ECDSA_Curve_NID, POINT_CONV_FORM>;

  static constexpr eECDSAEncoding signatureBinaryDataFormat = P_ECDSASignatureBinaryDataFormat;

  ECDSASignature() = default;

  explicit ECDSASignature(byte_array::ConstByteArray binary_signature)
    : signature_ecdsa_ptr_{Convert(binary_signature, signatureBinaryDataFormat)}
    , signature_{std::move(binary_signature)}
  {}

  template <eECDSAEncoding BIN_FORMAT>
  using ECDSASignatureType = ECDSASignature<BIN_FORMAT, T_Hasher, P_ECDSA_Curve_NID>;

  template <eECDSAEncoding P_ECDSASignatureBinaryDataFormat2, typename T_Hasher2,
            int            P_ECDSA_Curve_NID2>
  friend class ECDSASignature;

  template <eECDSAEncoding BIN_FORMAT>
  explicit ECDSASignature(ECDSASignatureType<BIN_FORMAT> const &from)
    : hash_{from.hash_}
    , signature_ecdsa_ptr_{from.signature_ecdsa_ptr_}
    , signature_{BIN_FORMAT == signatureBinaryDataFormat
                     ? from.signature_
                     : Convert(signature_ecdsa_ptr_, signatureBinaryDataFormat)}
  {}

  template <eECDSAEncoding BIN_FORMAT>
  explicit ECDSASignature(ECDSASignatureType<BIN_FORMAT> &&from)
    : ECDSASignature{safeMoveConstruct(std::move(from))}
  {}

  template <eECDSAEncoding BIN_FORMAT>
  ECDSASignature &operator=(ECDSASignatureType<BIN_FORMAT> const &from)
  {
    *this = ECDSASignature(from);
    return *this;
  }

  template <eECDSAEncoding BIN_FORMAT>
  ECDSASignature &operator=(ECDSASignatureType<BIN_FORMAT> &&from)
  {
    *this = safeMoveConstruct(std::move(from));
    return *this;
  }

  byte_array::ConstByteArray const &hash() const
  {
    return hash_;
  }

  SharedPointerType<const ECDSA_SIG> SignatureECDSAPtr() const
  {
    return signature_ecdsa_ptr_;
  }

  byte_array::ConstByteArray const &signature() const
  {
    return signature_;
  }

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  static ECDSASignature Sign(PrivateKeyType<BIN_ENC, POINT_CONV_FORM> const &private_key,
                             byte_array::ConstByteArray const &              data_to_sign)
  {
    return ECDSASignature(private_key, data_to_sign, eBinaryDataType::data);
  }

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  static ECDSASignature SignHash(PrivateKeyType<BIN_ENC, POINT_CONV_FORM> const &private_key,
                                 byte_array::ConstByteArray const &              hash_to_sign)
  {
    return ECDSASignature(private_key, hash_to_sign, eBinaryDataType::hash);
  }

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  bool VerifyHash(PublicKeyType<BIN_ENC, POINT_CONV_FORM> const &public_key,
                  byte_array::ConstByteArray const &             hash_to_verify) const
  {
    const int res =
        ECDSA_do_verify(static_cast<const uint8_t *>(hash_to_verify.pointer()),
                        static_cast<int>(hash_to_verify.size()), signature_ecdsa_ptr_.get(),
                        const_cast<EC_KEY *>(public_key.key().get()));

    switch (res)
    {
    case 1:
      return true;

    case 0:
      return false;

    case -1:
    default:
      throw std::runtime_error("VerifyHash(): ECDSA_do_verify(...) failed.");
    }
  }

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  bool Verify(PublicKeyType<BIN_ENC, POINT_CONV_FORM> const &public_key,
              byte_array::ConstByteArray const &             data_to_verify) const
  {
    return VerifyHash(public_key, Hash<HasherType>(data_to_verify));
  }

private:
  using AffineCoordConversionType = ECDSAAffineCoordinatesConversion<P_ECDSA_Curve_NID>;

  enum class eBinaryDataType : int
  {
    hash,
    data
  };

  //* For safe (noexcept) moving semantic constuctor
  ECDSASignature(byte_array::ConstByteArray &&  hash,
                 SharedPointerType<ECDSA_SIG> &&SignatureECDSAPtr,
                 byte_array::ConstByteArray &&  signature)
    : hash_{std::move(hash)}
    , signature_ecdsa_ptr_{std::move(SignatureECDSAPtr)}
    , signature_{std::move(signature)}
  {}

  template <eECDSAEncoding BIN_FORMAT>
  static ECDSASignature safeMoveConstruct(ECDSASignatureType<BIN_FORMAT> &&from)
  {
    byte_array::ConstByteArray signature{
        Convert(from.signature_ecdsa_ptr_, signatureBinaryDataFormat)};

    return ECDSASignature{std::move(from.hash_), std::move(from.signature_ecdsa_ptr_),
                          std::move(signature)};
  }

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  ECDSASignature(PrivateKeyType<BIN_ENC, POINT_CONV_FORM> const &private_key,
                 byte_array::ConstByteArray const &              data_to_sign,
                 const eBinaryDataType                           DataType = eBinaryDataType::data)
    : hash_{DataType == eBinaryDataType::data ? Hash<HasherType>(data_to_sign) : data_to_sign}
    , signature_ecdsa_ptr_{CreateSignature(private_key, hash_)}
    , signature_{Convert(signature_ecdsa_ptr_, signatureBinaryDataFormat)}
  {}

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  static SharedPointerType<ECDSA_SIG> CreateSignature(
      PrivateKeyType<BIN_ENC, POINT_CONV_FORM> const &private_key,
      byte_array::ConstByteArray const &              hash)
  {
    SharedPointerType<ECDSA_SIG> signature{
        ECDSA_do_sign(static_cast<const uint8_t *>(hash.pointer()), static_cast<int>(hash.size()),
                      const_cast<EC_KEY *>(private_key.key().get()))};

    if (!signature)
    {
      throw std::runtime_error("CreateSignature(..., hash, ...): ECDSA_do_sign(...) failed.");
    }

    return signature;
  }

  static byte_array::ByteArray ConvertDER(SharedPointerType<const ECDSA_SIG> &&signature)
  {
    byte_array::ByteArray der_sig;
    auto const est_size = static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), nullptr));
    der_sig.Resize(est_size);

    if (est_size < 1)
    {
      throw std::runtime_error(
          "Convert2Bin<...,eECDSAEncoding::DER,...>(): "
          "i2d_ECDSA_SIG(..., nullptr) failed.");
    }

    auto *     der_sig_ptr = static_cast<uint8_t *>(der_sig.pointer());
    auto const res_size    = static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), &der_sig_ptr));

    if (res_size < 1)
    {
      throw std::runtime_error(
          "Convert2Bin<...,eECDSAEncoding::DER,...(): "
          "i2d_ECDSA_SIG(..., &ptr) failed.");
    }
    if (res_size > est_size)
    {
      throw std::runtime_error(
          "Convert2Bin<...,eECDSAEncoding::DER,...(): i2d_ECDSA_SIG(..., &ptr) "
          "returned bigger DER "
          "signature size then originally anticipated for allocation.");
    }

    der_sig.Resize(res_size);

    return der_sig;
  }

  static UniquePointerType<ECDSA_SIG> ConvertDER(byte_array::ConstByteArray const &bin_sig)
  {
    auto const *der_sig_ptr = static_cast<const uint8_t *>(bin_sig.pointer());

    UniquePointerType<ECDSA_SIG> signature{
        d2i_ECDSA_SIG(nullptr, &der_sig_ptr, static_cast<long>(bin_sig.size()))};
    if (!signature)
    {
      throw std::runtime_error(
          "ConvertDER(const byte_array::ConstByteArray&): "
          "d2i_ECDSA_SIG(...) failed.");
    }

    return signature;
  }

  static byte_array::ByteArray ConvertCanonical(SharedPointerType<const ECDSA_SIG> &&signature)
  {
    BIGNUM const *r;
    BIGNUM const *s;
    ECDSA_SIG_get0(signature.get(), &r, &s);

    return AffineCoordConversionType::Convert2Canonical(r, s);
  }

  static UniquePointerType<ECDSA_SIG> ConvertCanonical(byte_array::ConstByteArray const &bin_sig)
  {
    UniquePointerType<BIGNUM, memory::eDeleteStrategy::clearing> r{BN_new()};
    UniquePointerType<BIGNUM, memory::eDeleteStrategy::clearing> s{BN_new()};

    AffineCoordConversionType::ConvertFromCanonical(bin_sig, r.get(), s.get());

    UniquePointerType<ECDSA_SIG> signature{ECDSA_SIG_new()};
    if (ECDSA_SIG_set0(signature.get(), r.get(), s.get()) == 0)
    {
      throw std::runtime_error(
          "ConvertCanonical<...,eECDSAEncoding::DER,...>("
          "const byte_array::ConstByteArray&): "
          "d2i_ECDSA_SIG(...) failed.");
    }

    r.release();
    s.release();

    return signature;
  }

  static byte_array::ByteArray Convert(SharedPointerType<const ECDSA_SIG> &&signature,
                                       eECDSAEncoding ouput_signature_binary_data_dype)
  {
    switch (ouput_signature_binary_data_dype)
    {
    case eECDSAEncoding::canonical:
    case eECDSAEncoding::bin:
      return ConvertCanonical(std::move(signature));

    case eECDSAEncoding::DER:
      return ConvertDER(std::move(signature));
    }

    return {};
  }

  static UniquePointerType<ECDSA_SIG> Convert(byte_array::ConstByteArray const &bin_sig,
                                              eECDSAEncoding input_signature_binary_data_type)
  {
    switch (input_signature_binary_data_type)
    {
    case eECDSAEncoding::canonical:
    case eECDSAEncoding::bin:
      return ConvertCanonical(bin_sig);

    case eECDSAEncoding::DER:
      return ConvertDER(bin_sig);
    }

    return {};
  }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
  static void ECDSA_SIG_get0(ECDSA_SIG const *sig, BIGNUM const **pr, BIGNUM const **ps)
  {
    if (pr != nullptr)
    {
      *pr = sig->r;
    }
    if (ps != nullptr)
    {
      *ps = sig->s;
    }
  }

  static int ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s)
  {
    if (r == nullptr || s == nullptr)
    {
      return 0;
    }
    BN_clear_free(sig->r);
    BN_clear_free(sig->s);
    sig->r = r;
    sig->s = s;
    return 1;
  }
#endif
};

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
