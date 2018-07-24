#ifndef CRYPTO_ECDSA_SIGNATURE_HPP
#define CRYPTO_ECDSA_SIGNATURE_HPP

#include "crypto/sha256.hpp"
#include "crypto/openssl_ecdsa_private_key.hpp"

#include <openssl/ecdsa.h>


namespace fetch {
namespace crypto {
namespace openssl {



template<typename T_Hasher = SHA256
       , int P_ECDSA_Curve_NID = NID_secp256k1
       , eECDSASignatureBinaryDataFormat P_ECDSASignatureBinaryDataFormat = eECDSASignatureBinaryDataFormat::canonical
       , point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
class ECDSASignature
{
    enum class eBinaryDataType : int
    {
        hash,
        data
    };

public:
    using ThisType = ECDSASignature;
    using hasher_type = T_Hasher;
    using ecdsa_curve_type = ECDSACurve<P_ECDSA_Curve_NID>;
    using public_key_type = ECDSAPublicKey<P_ECDSA_Curve_NID, P_ConversionForm>;
    using private_key_type = ECDSAPrivateKey<P_ECDSA_Curve_NID, P_ConversionForm>;

    static constexpr point_conversion_form_t conversionForm = P_ConversionForm;
    static constexpr eECDSASignatureBinaryDataFormat signatureBinaryDataFormat = P_ECDSASignatureBinaryDataFormat;

private:

    const byte_array::ConstByteArray hash_;
    const shrd_ptr_type<ECDSA_SIG> signature_ECDSA_SIG_;
    const byte_array::ConstByteArray signature_;

    static const std::size_t r_size_;
    static const std::size_t s_size_;

    static byte_array::ByteArray hash(byte_array::ConstByteArray const &data_to_sign) {
        T_Hasher hasher;
        hasher.Reset();
        hasher.Update(data_to_sign);
        hasher.Final();
        return hasher.digest();
    }

    static shrd_ptr_type<ECDSA_SIG> CreateSignature (
        private_key_type const &private_key,
        byte_array::ConstByteArray const &hash) {

        shrd_ptr_type<ECDSA_SIG> signature {
            ECDSA_do_sign(
                static_cast<const unsigned char *>(hash.pointer()),
                static_cast<int>(hash.size()),
                const_cast<EC_KEY*>(private_key.key().get()))};

        if (!signature) {
            throw std::runtime_error("CreateSignature(..., hash, ...): ECDSA_do_sign(...) failed.");
        }

        return signature;
    }



    byte_array::ByteArray
    ConvertCanonical(const shrd_ptr_type<const ECDSA_SIG> signature) {
        const std::size_t rBytes = static_cast<std::size_t>(BN_num_bytes(signature.get()->r));
        const std::size_t sBytes = static_cast<std::size_t>(BN_num_bytes(signature.get()->s));

        byte_array::ByteArray canonical_sig;
        canonical_sig.Resize(ecdsa_curve_type::signatureSize);

        for (std::size_t i = 0; i<(r_size_-rBytes); ++i) {
            canonical_sig[i] = 0;
        }

        if (!BN_bn2bin(
                signature.get()->r,
                static_cast<unsigned char *>(canonical_sig.pointer()) + r_size_ - rBytes)) {
            throw std::runtime_error("Convert2Bin<...,eECDSASignatureBinaryDataFormat::canonical,...>(): BN_bn2bin(r, ...) failed.");
        }

        for (std::size_t i = r_size_; i< (ecdsa_curve_type::signatureSize - sBytes); ++i) {
            canonical_sig[i] = 0;
        }

        if (!BN_bn2bin(
                signature.get()->s,
                static_cast<unsigned char *>(canonical_sig.pointer()) + r_size_ + s_size_ - sBytes)) {
            throw std::runtime_error("Convert2Bin<...,eECDSASignatureBinaryDataFormat::canonical,...>(): BN_bn2bin(r, ...) failed.");
        }

        return canonical_sig;
    }



    byte_array::ByteArray
    ConvertDER(const shrd_ptr_type<const ECDSA_SIG> signature) {
        byte_array::ByteArray der_sig;
        const std::size_t est_size = static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), nullptr));
        der_sig.Resize(est_size);

        if (est_size < 1) {
            throw std::runtime_error("Convert2Bin<...,eECDSASignatureBinaryDataFormat::DER,...>(): i2d_ECDSA_SIG(..., nullptr) failed.");
        }

        unsigned char* der_sig_ptr = static_cast<unsigned char*>(der_sig.pointer());
        const std::size_t res_size = static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), &der_sig_ptr));

        if (res_size < 1) {
            throw std::runtime_error("Convert2Bin<...,eECDSASignatureBinaryDataFormat::DER,...(): i2d_ECDSA_SIG(..., &ptr) failed.");
        } else if (res_size > est_size) {
            throw std::runtime_error("Convert2Bin<...,eECDSASignatureBinaryDataFormat::DER,...(): i2d_ECDSA_SIG(..., &ptr) returned bigger DER signature size then originally anticipated for allocation.");
        }

        der_sig.Resize(res_size);

        return der_sig;
    }

    byte_array::ByteArray
    Convert(
        const shrd_ptr_type<const ECDSA_SIG> signature,
        eECDSASignatureBinaryDataFormat ouput_signature_binary_data_type) {

        switch(ouput_signature_binary_data_type) {
            case eECDSASignatureBinaryDataFormat::canonical:
                return ConvertCanonical(signature);
            case eECDSASignatureBinaryDataFormat::DER:
                return ConvertDER(signature);
        }
    }

    uniq_ptr_type<ECDSA_SIG>
    ConvertCanonical (const byte_array::ConstByteArray& bin_sig) {
        uniq_ptr_type<ECDSA_SIG> signature {ECDSA_SIG_new()};

        if (!signature) {
            throw std::runtime_error("Convert<...,eECDSASignatureBinaryDataFormat::canonical,...>(const byte_array::ConstByteArray&): ECDSA_SIG_new() failed.");
        }

        if (!BN_bin2bn(bin_sig.pointer(), static_cast<int>(r_size_), signature.get()->r)) {
            throw std::runtime_error("Convert<...,eECDSASignatureBinaryDataFormat::canonical,...>(const byte_array::ConstByteArray&): i2d_ECDSA_SIG(..., r) failed.");
        }

        if (!BN_bin2bn(bin_sig.pointer() + r_size_, static_cast<int>(s_size_), signature.get()->s)) {
            throw std::runtime_error("Convert<...,eECDSASignatureBinaryDataFormat::canonical,...>(const byte_array::ConstByteArray&): i2d_ECDSA_SIG(..., s) failed.");
        }

        return signature;
    }



    uniq_ptr_type<ECDSA_SIG>
    ConvertDER(const byte_array::ConstByteArray& bin_sig) {
        const unsigned char *der_sig_ptr = static_cast<const unsigned char *>(bin_sig.pointer());

        uniq_ptr_type<ECDSA_SIG> signature {d2i_ECDSA_SIG(nullptr, &der_sig_ptr, static_cast<long>(bin_sig.size()))};
        if (!signature) {
            throw std::runtime_error("Convert<...,eECDSASignatureBinaryDataFormat::DER,...>(const byte_array::ConstByteArray&): d2i_ECDSA_SIG(...) failed.");
        }

        return signature;
    }

    uniq_ptr_type<ECDSA_SIG>
    Convert(
        const byte_array::ConstByteArray& bin_sig,
        eECDSASignatureBinaryDataFormat input_signature_binary_data_type) {

        switch(input_signature_binary_data_type) {
            case eECDSASignatureBinaryDataFormat::canonical:
                return ConvertCanonical(bin_sig);
            case eECDSASignatureBinaryDataFormat::DER:
                return ConvertDER(bin_sig);
        }
    }

    ECDSASignature(
        private_key_type const &private_key,
        byte_array::ConstByteArray const &data_to_sign,
        const eBinaryDataType data_type = eBinaryDataType::data)
        : hash_{data_type == eBinaryDataType::data ? hash(data_to_sign) : byte_array::ByteArray()}
        , signature_ECDSA_SIG_{CreateSignature(private_key, hash_)}
        , signature_{Convert(signature_ECDSA_SIG_, signatureBinaryDataFormat)} {
    }

public:

    ECDSASignature(ECDSASignature const &from) = default;
    ECDSASignature(ECDSASignature &&from) = default;

    ECDSASignature(byte_array::ConstByteArray const &binary_signature)
        : hash_{}
        , signature_ECDSA_SIG_{Convert(binary_signature, signatureBinaryDataFormat)}
        , signature_{binary_signature} {
    }



    static ECDSASignature Sign(
        private_key_type const &private_key,
        byte_array::ConstByteArray const &data_to_sign) {

        return ECDSASignature(private_key, data_to_sign, eBinaryDataType::data);
    }

    static ECDSASignature SignHash(
        private_key_type const &private_key,
        byte_array::ConstByteArray const &hash_to_sign) {

        return ECDSASignature(private_key, hash_to_sign, eBinaryDataType::hash);
    }

    bool VerifyHash(
        public_key_type const &public_key,
        byte_array::ConstByteArray const &hash_to_verify) {

        if (hash_.size() > 0) {
            return hash_to_verify == hash_;
        }

        const int res =
            ECDSA_do_verify(
                static_cast<const unsigned char *>(hash_to_verify.pointer()),
                static_cast<int>(hash_to_verify.size()),
                signature_ECDSA_SIG_.get(),
                const_cast<EC_KEY*>(public_key.key().get()));

        switch (res) {
            case 1:
                return true;

            case 0:
                return false;

            case -1:
            default:
                throw std::runtime_error("ecdsa_verify(): ECDSA_verify(...) failed.");
        }
    }

    bool Verify(
        public_key_type const &public_key,
        byte_array::ConstByteArray const &data_to_verify) {

        return VerifyHash(public_key, hash(data_to_verify));
    }
};



template<typename T_Hasher
       , int P_ECDSA_Curve_NID
       , eECDSASignatureBinaryDataFormat P_ECDSASignatureBinaryDataFormat
       , point_conversion_form_t P_ConversionForm>
const std::size_t ECDSASignature<T_Hasher, P_ECDSA_Curve_NID, P_ECDSASignatureBinaryDataFormat, P_ConversionForm>::r_size_ = ecdsa_curve_type::signatureSize>>1;

template<typename T_Hasher
       , int P_ECDSA_Curve_NID
       , eECDSASignatureBinaryDataFormat P_ECDSASignatureBinaryDataFormat
       , point_conversion_form_t P_ConversionForm>
const std::size_t ECDSASignature<T_Hasher, P_ECDSA_Curve_NID, P_ECDSASignatureBinaryDataFormat, P_ConversionForm>::s_size_ = ECDSASignature<T_Hasher, P_ECDSA_Curve_NID, P_ECDSASignatureBinaryDataFormat, P_ConversionForm>::r_size_;



namespace detail {
} //namespace


} //namespace openssl
} //namespace crypto
} //namespace fetch

#endif //CRYPTO_ECDSA_SIGNATURE_HPP
