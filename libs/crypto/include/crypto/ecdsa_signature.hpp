#ifndef CRYPTO_ECDSA_SIGNATURE_HPP
#define CRYPTO_ECDSA_SIGNATURE_HPP

#include "crypto/sha256.hpp"
#include "crypto/openssl_ecdsa_private_key.hpp"

#include <openssl/ecdsa.h>


namespace fetch {
namespace crypto {
namespace openssl {

enum eECDSASignatureFormat : int
{
    canonical,
    DER
};

template<typename T_Hasher = SHA256
       , int P_ECDSA_Curve_NID = NID_secp256k1
       , eECDSASignatureFormat P_ECDSASignatureFormat = eECDSASignatureFormat::canonical
       , point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
class ECDSASignature
{
public:
    using del_strat_type = memory::eDeleteStrategy;
    
    template <typename T
            , del_strat_type P_DeleteStrategy = del_strat_type::canonical>
    using shrd_ptr_type = memory::ossl_shared_ptr<T, P_DeleteStrategy>;

    template <typename T
            , del_strat_type P_DeleteStrategy = del_strat_type::canonical>
    using uniq_ptr_type = memory::ossl_unique_ptr<T, P_DeleteStrategy>;

    using ecdsa_curve_type = ECDSACurve<P_ECDSA_Curve_NID>;
    using public_key_type = ECDSAPublicKey<P_ECDSA_Curve_NID, P_ConversionForm>;
    using private_key_type = ECDSAPrivateKey<P_ECDSA_Curve_NID, P_ConversionForm>;

    static constexpr point_conversion_form_t conversionForm = P_ConversionForm;
    static constexpr std::size_t r_size_ = 20;
    static constexpr std::size_t s_size_ = 20;

private:
    const byte_array::ConstByteArray signature_;
    const byte_array::ConstByteArray hash_binary_;
    const shrd_ptr_type<ECDSA_SIG> hash_ECDSA_SIG_;

    static byte_array::ByteArray hash (
        byte_array::ConstByteArray const &data_to_sign) {

        T_Hasher hasher;
        hasher.Reset();
        hasher.Update(data_to_sign);
        hasher.Final();
        return hasher.digest();
    }

    static shrd_ptr_type<ECDSA_SIG> CreateSignature (
        private_key_type const &private_key,
        byte_array::ConstByteArray const &hash) {

        shrd_ptr_type<ECDSA_SIG> signature = 
            ECDSA_do_sign (
                static_cast<const unsigned char *>(hash.pointer()),
                static_cast<int>(hash.size()),
                const_cast<EC_KEY*>(private_key.key().get()));

        if (!signature) {
            throw std::runtime_error("CreateSignature(..., hash, ...): ECDSA_do_sign(...) failed.");
        }

        return signature;
    }

    static array::ByteArray Convert2 (
        const shrd_ptr_type<const ECDSA_SIG> signature)
        );

    static shrd_ptr_type<ECDSA_SIG> Convert (
        const array::ConstByteArray& bin_sig)
        );

public:

    ECDSASignature()
}


template<typename T_Hasher = SHA256
    , int P_ECDSA_Curve_NID = NID_secp256k1
    , point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
byte_array::ByteArray
ECDSASnature<T_Hasher, P_ECDSA_Curve_NID, eECDSASignatureFormat::canonical, P_ConversionForm>::Convert (
    const shrd_ptr_type<const ECDSA_SIG> signature) {

    const std::size_t rBytes = static_cast<std::size_t>(BN_num_bytes(signature.get()->r));
    const std::size_t sBytes = static_cast<std::size_t>(BN_num_bytes(signature.get()->s));

    static const char signature_zeroed_array[r_size_ + s_size_] = {0};
    array::ByteArray canonical_sig = signature_zeroed_array;

    if (!BN_bn2bin(
            signature.get()->r + r_size_ - rBytes,
            static_cast<unsigned char *>(canonical_sig.pointer()))) {
        throw std::runtime_error("Convert2Bin<...,eECDSASignatureFormat::canonical,...>(): BN_bn2bin(r, ...) failed.");
    }

    if (!BN_bn2bin(
            signature.get()->s + r_size_ + s_size_ - sBytes,
            static_cast<unsigned char *>(canonical_sig.pointer()))) {
        throw std::runtime_error("Convert2Bin<...,eECDSASignatureFormat::canonical,...>(): BN_bn2bin(r, ...) failed.");
    }

    return canonical_sig;
}

template<typename T_Hasher = SHA256
    , int P_ECDSA_Curve_NID = NID_secp256k1
    , point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
byste_array::ByteArray
ECDSASnature<T_Hasher, P_ECDSA_Curve_NID, eECDSASignatureFormat::DER, P_ConversionForm>::Convert (
    const shrd_ptr_type<const ECDSA_SIG> signature) {

    byte_array::ByteArray der_sig;
    const std::size_t est_size = static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), nullptr));
    der_sig.Resize(est_len);

    if (est_size < 1) {
        throw std::runtime_error("Convert2Bin<...,eECDSASignatureFormat::DER,...>(): i2d_ECDSA_SIG(..., nullptr) failed.");
    }

    unsigned char* der_sig_ptr = static_cast<unsigned char*>(der_sig.pointer());
    const std::size_t res_size = static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), &der_sig_ptr));
    
    if (res_size < 1) {
        throw std::runtime_error("Convert2Bin<...,eECDSASignatureFormat::DER,...(): i2d_ECDSA_SIG(..., &ptr) failed.");
    } else if (res_len > res_size) {
        throw std::runtime_error("Convert2Bin<...,eECDSASignatureFormat::DER,...(): i2d_ECDSA_SIG(..., &ptr) returned bigger DER signature size then originally anticipated for allocation.");
    }

    der_sig.Resize(res_size);
    
    return der_sig;
}

template<typename T_Hasher = SHA256
    , int P_ECDSA_Curve_NID = NID_secp256k1
    , point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
shrd_ptr_type<ECDSA_SIG>
ECDSASnature<T_Hasher, P_ECDSA_Curve_NID, eECDSASignatureFormat::canonical, P_ConversionForm>::Convert (
    const byte_array::ConstByteArray& bin_sig) {

    shrd_ptr_type<ECDSA_SIG> signature = ECDSA_SIG_new();

    if (!signature) {
        throw std::runtime_error("Convert<...,eECDSASignatureFormat::canonical,...>(const byte_array::ConstByteArray&): ECDSA_SIG_new() failed.");
    }

    if (!BN_bin2bn(bin_sig.get()          , r_size_, signature.get()->r)) {
        throw std::runtime_error("Convert<...,eECDSASignatureFormat::canonical,...>(const byte_array::ConstByteArray&): i2d_ECDSA_SIG(..., r) failed.");
    }

    if (!BN_bin2bn(bin_sig.get() + r_size_, s_size_, signature.get()->s)) {
        throw std::runtime_error("Convert<...,eECDSASignatureFormat::canonical,...>(const byte_array::ConstByteArray&): i2d_ECDSA_SIG(..., s) failed.");
    }

    return signature;
}

template<typename T_Hasher = SHA256
    , int P_ECDSA_Curve_NID = NID_secp256k1
    , point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
shrd_ptr_type<ECDSA_SIG>
ECDSASnature<T_Hasher, P_ECDSA_Curve_NID, eECDSASignatureFormat::DER, P_ConversionForm>::Convert (
    const byte_array::ConstByteArray& bin_sig {
    
    const unsigned char *der_sig_ptr = static_cast<const unsigned char *>(bin_sig.pointer());

    shrd_ptr_type<ECDSA_SIG> signature {d2i_ECDSA_SIG(nullptr, &der_sig_ptr, static_cast<long>(bin_sig.size())};
    if (!signature) {
        throw std::runtime_error("Convert<...,eECDSASignatureFormat::DER,...>(const byte_array::ConstByteArray&): d2i_ECDSA_SIG(...) failed.");
    }

    return signature;
}

} //namespace openssl
} //namespace crypto
} //namespace fetch

#endif //CRYPTO_ECDSA_SIGNATURE_HPP
