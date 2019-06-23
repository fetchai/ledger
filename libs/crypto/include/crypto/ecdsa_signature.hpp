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
  byte_array::ConstByteArray hash_;
  shrd_ptr_type<ECDSA_SIG>   signature_ECDSA_SIG_;
  byte_array::ConstByteArray signature_;

public:
  using hasher_type      = T_Hasher;
  using ecdsa_curve_type = ECDSACurve<P_ECDSA_Curve_NID>;
  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  using public_key_type = ECDSAPublicKey<BIN_ENC, P_ECDSA_Curve_NID, POINT_CONV_FORM>;
  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  using private_key_type = ECDSAPrivateKey<BIN_ENC, P_ECDSA_Curve_NID, POINT_CONV_FORM>;

  static constexpr eECDSAEncoding signatureBinaryDataFormat = P_ECDSASignatureBinaryDataFormat;

  ECDSASignature() = default;

  ECDSASignature(byte_array::ConstByteArray binary_signature)
    : hash_{}
    , signature_ECDSA_SIG_{Convert(binary_signature, signatureBinaryDataFormat)}
    , signature_{std::move(binary_signature)}
  {}

  template <eECDSAEncoding BIN_FORMAT>
  using ecdsa_signature_type = ECDSASignature<BIN_FORMAT, T_Hasher, P_ECDSA_Curve_NID>;

  template <eECDSAEncoding P_ECDSASignatureBinaryDataFormat2, typename T_Hasher2,
            int            P_ECDSA_Curve_NID2>
  friend class ECDSASignature;

  template <eECDSAEncoding BIN_FORMAT>
  ECDSASignature(ecdsa_signature_type<BIN_FORMAT> const &from)
    : hash_{from.hash_}
    , signature_ECDSA_SIG_{from.signature_ECDSA_SIG_}
    , signature_{BIN_FORMAT == signatureBinaryDataFormat
                     ? from.signature_
                     : Convert(signature_ECDSA_SIG_, signatureBinaryDataFormat)}
  {}

  // ECDSASignature(ECDSASignature&& from) = default;

  template <eECDSAEncoding BIN_FORMAT>
  ECDSASignature(ecdsa_signature_type<BIN_FORMAT> &&from)
    : ECDSASignature{safeMoveConstruct(std::move(from))}
  {}

  // ECDSASignature& operator = (ECDSASignature const & from) = default;

  template <eECDSAEncoding BIN_FORMAT>
  ECDSASignature operator=(ecdsa_signature_type<BIN_FORMAT> const &from)
  {
    *this = ECDSASignature(from);
    return *this;
  }

  template <eECDSAEncoding BIN_FORMAT>
  ECDSASignature operator=(ecdsa_signature_type<BIN_FORMAT> &&from)
  {
    *this = safeMoveConstruct(std::move(from));
    return *this;
  }

  const byte_array::ConstByteArray &hash() const
  {
    return hash_;
  }

  shrd_ptr_type<const ECDSA_SIG> signature_ECDSA_SIG() const
  {
    return signature_ECDSA_SIG_;
  }

  const byte_array::ConstByteArray &signature() const
  {
    return signature_;
  }

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  static ECDSASignature Sign(private_key_type<BIN_ENC, POINT_CONV_FORM> const &private_key,
                             byte_array::ConstByteArray const &                data_to_sign)
  {
    return ECDSASignature(private_key, data_to_sign, eBinaryDataType::data);
  }

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  static ECDSASignature SignHash(private_key_type<BIN_ENC, POINT_CONV_FORM> const &private_key,
                                 byte_array::ConstByteArray const &                hash_to_sign)
  {
    return ECDSASignature(private_key, hash_to_sign, eBinaryDataType::hash);
  }

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  bool VerifyHash(public_key_type<BIN_ENC, POINT_CONV_FORM> const &public_key,
                  byte_array::ConstByteArray const &               hash_to_verify) const
  {
    const int res =
        ECDSA_do_verify(static_cast<const unsigned char *>(hash_to_verify.pointer()),
                        static_cast<int>(hash_to_verify.size()), signature_ECDSA_SIG_.get(),
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
  bool Verify(public_key_type<BIN_ENC, POINT_CONV_FORM> const &public_key,
              byte_array::ConstByteArray const &               data_to_verify) const
  {
    return VerifyHash(public_key, Hash<hasher_type>(data_to_verify));
  }

private:
  using affine_coord_conversion_type = ECDSAAffineCoordinatesConversion<P_ECDSA_Curve_NID>;

  enum class eBinaryDataType : int
  {
    hash,
    data
  };

  //* For safe (noexcept) moving semantic constuctor
  ECDSASignature(byte_array::ConstByteArray &&hash, shrd_ptr_type<ECDSA_SIG> &&signature_ECDSA_SIG,
                 byte_array::ConstByteArray &&signature)
    : hash_{std::move(hash)}
    , signature_ECDSA_SIG_{std::move(signature_ECDSA_SIG)}
    , signature_{std::move(signature)}
  {}

  template <eECDSAEncoding BIN_FORMAT>
  static ECDSASignature safeMoveConstruct(ecdsa_signature_type<BIN_FORMAT> &&from)
  {
    byte_array::ConstByteArray signature{
        Convert(from.signature_ECDSA_SIG_, signatureBinaryDataFormat)};

    return ECDSASignature{std::move(from.hash_), std::move(from.signature_ECDSA_SIG_),
                          std::move(signature)};
  }

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  ECDSASignature(private_key_type<BIN_ENC, POINT_CONV_FORM> const &private_key,
                 byte_array::ConstByteArray const &                data_to_sign,
                 const eBinaryDataType data_type = eBinaryDataType::data)
    : hash_{data_type == eBinaryDataType::data ? Hash<hasher_type>(data_to_sign) : data_to_sign}
    , signature_ECDSA_SIG_{CreateSignature(private_key, hash_)}
    , signature_{Convert(signature_ECDSA_SIG_, signatureBinaryDataFormat)}
  {}

  template <eECDSAEncoding BIN_ENC, point_conversion_form_t POINT_CONV_FORM>
  static shrd_ptr_type<ECDSA_SIG> CreateSignature(
      private_key_type<BIN_ENC, POINT_CONV_FORM> const &private_key,
      byte_array::ConstByteArray const &                hash)
  {
    shrd_ptr_type<ECDSA_SIG> signature{ECDSA_do_sign(
        static_cast<const unsigned char *>(hash.pointer()), static_cast<int>(hash.size()),
        const_cast<EC_KEY *>(private_key.key().get()))};

    if (!signature)
    {
      throw std::runtime_error("CreateSignature(..., hash, ...): ECDSA_do_sign(...) failed.");
    }

    return signature;
  }

  static byte_array::ByteArray ConvertDER(shrd_ptr_type<const ECDSA_SIG> &&signature)
  {
    byte_array::ByteArray der_sig;
    const std::size_t est_size = static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), nullptr));
    der_sig.Resize(est_size);

    if (est_size < 1)
    {
      throw std::runtime_error(
          "Convert2Bin<...,eECDSAEncoding::DER,...>(): "
          "i2d_ECDSA_SIG(..., nullptr) failed.");
    }

    unsigned char *   der_sig_ptr = static_cast<unsigned char *>(der_sig.pointer());
    const std::size_t res_size =
        static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), &der_sig_ptr));

    if (res_size < 1)
    {
      throw std::runtime_error(
          "Convert2Bin<...,eECDSAEncoding::DER,...(): "
          "i2d_ECDSA_SIG(..., &ptr) failed.");
    }
    else if (res_size > est_size)
    {
      throw std::runtime_error(
          "Convert2Bin<...,eECDSAEncoding::DER,...(): i2d_ECDSA_SIG(..., &ptr) "
          "returned bigger DER "
          "signature size then originally anticipated for allocation.");
    }

    der_sig.Resize(res_size);

    return der_sig;
  }

  static uniq_ptr_type<ECDSA_SIG> ConvertDER(const byte_array::ConstByteArray &bin_sig)
  {
    const unsigned char *der_sig_ptr = static_cast<const unsigned char *>(bin_sig.pointer());

    uniq_ptr_type<ECDSA_SIG> signature{
        d2i_ECDSA_SIG(nullptr, &der_sig_ptr, static_cast<long>(bin_sig.size()))};
    if (!signature)
    {
      throw std::runtime_error(
          "ConvertDER(const byte_array::ConstByteArray&): "
          "d2i_ECDSA_SIG(...) failed.");
    }

    return signature;
  }

  static byte_array::ByteArray ConvertCanonical(shrd_ptr_type<const ECDSA_SIG> &&signature)
  {
    BIGNUM const *r;
    BIGNUM const *s;
    ECDSA_SIG_get0(signature.get(), &r, &s);

    return affine_coord_conversion_type::Convert2Canonical(r, s);
  }

  static uniq_ptr_type<ECDSA_SIG> ConvertCanonical(const byte_array::ConstByteArray &bin_sig)
  {
    uniq_ptr_type<BIGNUM, memory::eDeleteStrategy::clearing> r{BN_new()};
    uniq_ptr_type<BIGNUM, memory::eDeleteStrategy::clearing> s{BN_new()};

    affine_coord_conversion_type::ConvertFromCanonical(bin_sig, r.get(), s.get());

    uniq_ptr_type<ECDSA_SIG> signature{ECDSA_SIG_new()};
    if (!ECDSA_SIG_set0(signature.get(), r.get(), s.get()))
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

  static byte_array::ByteArray Convert(shrd_ptr_type<const ECDSA_SIG> &&signature,
                                       eECDSAEncoding ouput_signature_binary_data_type)
  {
    switch (ouput_signature_binary_data_type)
    {
    case eECDSAEncoding::canonical:
    case eECDSAEncoding::bin:
      return ConvertCanonical(std::move(signature));

    case eECDSAEncoding::DER:
      return ConvertDER(std::move(signature));
    }

    return {};
  }

  static uniq_ptr_type<ECDSA_SIG> Convert(const byte_array::ConstByteArray &bin_sig,
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
  static void ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps)
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
