#pragma once

#include "crypto/sha256.hpp"
#include "crypto/openssl_ecdsa_private_key.hpp"
#include "crypto/hash.hpp"

#include <openssl/ecdsa.h>


namespace fetch {
namespace crypto {
namespace openssl {


template<eECDSABinaryDataFormat P_ECDSASignatureBinaryDataFormat = eECDSABinaryDataFormat::canonical
       , typename T_Hasher = SHA256
       , int P_ECDSA_Curve_NID = NID_secp256k1>
class ECDSASignature
{
    byte_array::ConstByteArray hash_;
    shrd_ptr_type<ECDSA_SIG> signature_ECDSA_SIG_;
    byte_array::ConstByteArray signature_;

public:
    using hasher_type = T_Hasher;
    using ecdsa_curve_type = ECDSACurve<P_ECDSA_Curve_NID>;
    template<eECDSABinaryDataFormat  BIN_ENC
           , point_conversion_form_t POINT_CONV_FORM>
    using public_key_type = ECDSAPublicKey<BIN_ENC, P_ECDSA_Curve_NID, POINT_CONV_FORM>;
    template<eECDSABinaryDataFormat  BIN_ENC
           , point_conversion_form_t POINT_CONV_FORM>
    using private_key_type = ECDSAPrivateKey<BIN_ENC, P_ECDSA_Curve_NID, POINT_CONV_FORM>;

    static constexpr eECDSABinaryDataFormat signatureBinaryDataFormat = P_ECDSASignatureBinaryDataFormat;


    ECDSASignature(byte_array::ConstByteArray const &binary_signature)
        : hash_{}
        , signature_ECDSA_SIG_{Convert(binary_signature, signatureBinaryDataFormat)}
        , signature_{binary_signature}
    {
    }


    template<eECDSABinaryDataFormat BIN_FORMAT>
    using ecdsa_signature_type = ECDSASignature<BIN_FORMAT, T_Hasher, P_ECDSA_Curve_NID>;

    template<eECDSABinaryDataFormat P_ECDSASignatureBinaryDataFormat2
           , typename T_Hasher2
           , int P_ECDSA_Curve_NID2>
    friend class ECDSASignature;


    template<eECDSABinaryDataFormat BIN_FORMAT>
    ECDSASignature(ecdsa_signature_type<BIN_FORMAT> const& from)
        : hash_{from.hash_}
        , signature_ECDSA_SIG_{from.signature_ECDSA_SIG_}
        , signature_{BIN_FORMAT == signatureBinaryDataFormat ? from.signature_ : Convert(signature_ECDSA_SIG_, signatureBinaryDataFormat)}
    {
    }


    ECDSASignature(ECDSASignature&& from) = default;

    template<eECDSABinaryDataFormat BIN_FORMAT>
    ECDSASignature(ecdsa_signature_type<BIN_FORMAT>&& from)
        : ECDSASignature{ safeMoveConstruct(std::move(from)) }
    {
    }



    const byte_array::ConstByteArray& hash() const
    {
        return hash_;
    }

    shrd_ptr_type<const ECDSA_SIG> signature_ECDSA_SIG() const
    {
        return signature_ECDSA_SIG_;
    }


    const byte_array::ConstByteArray & signature() const
    {
        return signature_;
    }


    template<eECDSABinaryDataFormat  BIN_ENC
           , point_conversion_form_t POINT_CONV_FORM>
    static ECDSASignature
    Sign(
        private_key_type<BIN_ENC, POINT_CONV_FORM> const & private_key,
        byte_array::ConstByteArray const & data_to_sign
        )
    {

        return ECDSASignature(private_key, data_to_sign, eBinaryDataType::data);
    }


    template<eECDSABinaryDataFormat  BIN_ENC
           , point_conversion_form_t POINT_CONV_FORM>
    static ECDSASignature
    SignHash(
        private_key_type<BIN_ENC, POINT_CONV_FORM> const & private_key,
        byte_array::ConstByteArray const & hash_to_sign
        )
    {
        return ECDSASignature(private_key, hash_to_sign, eBinaryDataType::hash);
    }


    template<eECDSABinaryDataFormat  BIN_ENC
           , point_conversion_form_t POINT_CONV_FORM>
    bool VerifyHash (
        public_key_type<BIN_ENC, POINT_CONV_FORM> const & public_key,
        byte_array::ConstByteArray const & hash_to_verify
        ) const
    {
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
                throw std::runtime_error("VerifyHash(): ECDSA_do_verify(...) failed.");
        }
    }

    template<eECDSABinaryDataFormat  BIN_ENC
           , point_conversion_form_t POINT_CONV_FORM>
   bool Verify (
        public_key_type<BIN_ENC, POINT_CONV_FORM> const & public_key,
        byte_array::ConstByteArray const & data_to_verify
        ) const
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
    ECDSASignature(
        byte_array::ConstByteArray&& hash,
        shrd_ptr_type<ECDSA_SIG>&& signature_ECDSA_SIG,
        byte_array::ConstByteArray&& signature
        )
        : hash_{ std::move(hash) }
        , signature_ECDSA_SIG_{ std::move(signature_ECDSA_SIG) }
        , signature_{ std::move(signature) }
    {
    }


    template<eECDSABinaryDataFormat BIN_FORMAT>
    static ECDSASignature
    safeMoveConstruct(ecdsa_signature_type<BIN_FORMAT>&& from)
    {
        byte_array::ConstByteArray signature{ Convert(from.signature_ECDSA_SIG_, signatureBinaryDataFormat) };

        return ECDSASignature{
            std::move(from.hash_),
            std::move(from.signature_ECDSA_SIG_),
            std::move(signature)};
    }


    template<eECDSABinaryDataFormat  BIN_ENC
           , point_conversion_form_t POINT_CONV_FORM>
    ECDSASignature(
        private_key_type<BIN_ENC, POINT_CONV_FORM> const & private_key,
        byte_array::ConstByteArray const & data_to_sign,
        const eBinaryDataType data_type = eBinaryDataType::data
        )
        : hash_{data_type == eBinaryDataType::data ? Hash<hasher_type>(data_to_sign) : byte_array::ByteArray()}
        , signature_ECDSA_SIG_{CreateSignature(private_key, hash_)}
        , signature_{Convert(signature_ECDSA_SIG_, signatureBinaryDataFormat)}
    {
    }


    template<eECDSABinaryDataFormat  BIN_ENC
           , point_conversion_form_t POINT_CONV_FORM>
    static shrd_ptr_type<ECDSA_SIG> 
    CreateSignature (
        private_key_type<BIN_ENC, POINT_CONV_FORM> const & private_key,
        byte_array::ConstByteArray const & hash
        )
    {
        shrd_ptr_type<ECDSA_SIG> signature {
            ECDSA_do_sign(
                static_cast<const unsigned char *>(hash.pointer()),
                static_cast<int>(hash.size()),
                const_cast<EC_KEY*>(private_key.key().get()))};

        if (!signature)
        {
            throw std::runtime_error("CreateSignature(..., hash, ...): ECDSA_do_sign(...) failed.");
        }

        return signature;
    }


    static byte_array::ByteArray ConvertDER(shrd_ptr_type<const ECDSA_SIG>&& signature)
    {
        byte_array::ByteArray der_sig;
        const std::size_t est_size = static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), nullptr));
        der_sig.Resize(est_size);

        if (est_size < 1) {
            throw std::runtime_error("Convert2Bin<...,eECDSABinaryDataFormat::DER,...>(): i2d_ECDSA_SIG(..., nullptr) failed.");
        }

        unsigned char* der_sig_ptr = static_cast<unsigned char*>(der_sig.pointer());
        const std::size_t res_size = static_cast<std::size_t>(i2d_ECDSA_SIG(signature.get(), &der_sig_ptr));

        if (res_size < 1)
        {
            throw std::runtime_error("Convert2Bin<...,eECDSABinaryDataFormat::DER,...(): i2d_ECDSA_SIG(..., &ptr) failed.");
        }
        else if (res_size > est_size)
        {
            throw std::runtime_error("Convert2Bin<...,eECDSABinaryDataFormat::DER,...(): i2d_ECDSA_SIG(..., &ptr) returned bigger DER signature size then originally anticipated for allocation.");
        }

        der_sig.Resize(res_size);

        return der_sig;
    }


    static uniq_ptr_type<ECDSA_SIG> ConvertDER(const byte_array::ConstByteArray& bin_sig)
    {
        const unsigned char *der_sig_ptr = static_cast<const unsigned char *>(bin_sig.pointer());

        uniq_ptr_type<ECDSA_SIG> signature {
            d2i_ECDSA_SIG(nullptr, &der_sig_ptr, static_cast<long>(bin_sig.size()))};
        if (!signature)
        {
            throw std::runtime_error("Convert<...,eECDSABinaryDataFormat::DER,...>(const byte_array::ConstByteArray&): d2i_ECDSA_SIG(...) failed.");
        }

        return signature;
    }


    static byte_array::ByteArray ConvertCanonical(shrd_ptr_type<const ECDSA_SIG>&& signature)
    {
        return affine_coord_conversion_type::Convert2Canonical(
            signature.get()->r, signature.get()->s);
    }


    static uniq_ptr_type<ECDSA_SIG> ConvertCanonical(const byte_array::ConstByteArray& bin_sig)
    {
        uniq_ptr_type<ECDSA_SIG> signature {ECDSA_SIG_new()};
        affine_coord_conversion_type::ConvertFromCanonical(bin_sig, signature.get()->r, signature.get()->s);
        return signature;
    }


    static byte_array::ByteArray Convert(
        shrd_ptr_type<const ECDSA_SIG>&& signature,
        eECDSABinaryDataFormat ouput_signature_binary_data_type
        )
    {
        switch(ouput_signature_binary_data_type)
        {
            case eECDSABinaryDataFormat::canonical:
            case eECDSABinaryDataFormat::bin:
                return ConvertCanonical(std::move(signature));

            case eECDSABinaryDataFormat::DER:
                return ConvertDER(std::move(signature));
        }
    }


    static uniq_ptr_type<ECDSA_SIG> Convert(
        const byte_array::ConstByteArray & bin_sig,
        eECDSABinaryDataFormat input_signature_binary_data_type
        )
    {
        switch(input_signature_binary_data_type)
        {
            case eECDSABinaryDataFormat::canonical:
            case eECDSABinaryDataFormat::bin:
                return ConvertCanonical(bin_sig);

            case eECDSABinaryDataFormat::DER:
                return ConvertDER(bin_sig);
        }
    }
};


} //namespace openssl
} //namespace crypto
} //namespace fetch
