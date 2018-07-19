#ifndef CRYPTO_OPENSSL_PUBLIC_KEY_HPP
#define CRYPTO_OPENSSL_PUBLIC_KEY_HPP

#include "core/byte_array/byte_array.hpp"
#include "crypto/openssl_common.hpp"
#include "crypto/openssl_memory.hpp"
#include "crypto/openssl_context_session.hpp"


namespace fetch {
namespace crypto {
namespace openssl {

template<
    int P_ECDSA_Curve_NID = NID_secp256k1,
    point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
class ECDSAPublicKey
{
public:
    using eDelStrat = memory::eDeleteStrategy;

    template <typename T
            , eDelStrat P_DeleteStrategy = eDelStrat::canonical>
    using ShrdPtr = memory::ossl_shared_ptr<T, P_DeleteStrategy>;

    template <typename T
            , eDelStrat P_DeleteStrategy = eDelStrat::canonical>
    using UniqPtr = memory::ossl_unique_ptr<T, P_DeleteStrategy>;

    using ECDSACurveType = ECDSACurve<P_ECDSA_Curve_NID>;

    static constexpr point_conversion_form_t conversionForm = P_ConversionForm;

private:
    ShrdPtr<EC_POINT> key_EC_POINT_;
    ShrdPtr<EC_KEY> key_EC_KEY_;
    const byte_array::ConstByteArray key_binary_;

    static byte_array::ByteArray convert(
        EC_POINT* public_key,
        const EC_GROUP *group,
        const context::Session<BN_CTX>& session) {

        ShrdPtr<BIGNUM> public_key_as_BN {BN_new()};
        if (!EC_POINT_point2bn(group, public_key, ECDSAPublicKey::conversionForm, public_key_as_BN.get(), session.context().get())) {
            throw std::runtime_error("ECDSAPublicKey::convert(...) failed due to failure of the `EC_POINT_point2bn(...)` function."); 
        }

        byte_array::ByteArray pub_key_as_bin;
        pub_key_as_bin.Resize(static_cast<std::size_t>( BN_num_bytes(public_key_as_BN.get())));

        if (!BN_bn2bin(public_key_as_BN.get(), static_cast<unsigned char *>(pub_key_as_bin.pointer()))) {
            throw std::runtime_error("ECDSAPublicKey::convert(...) failed due to failure of the `BN_bn2bin(...)` function.");
        }

        return pub_key_as_bin;
    }

    static UniqPtr<EC_POINT> convert(byte_array::ConstByteArray const& key_data) {
        ShrdPtr<BIGNUM> pub_key_as_BN {BN_new()};
        if (!BN_bin2bn(static_cast<const unsigned char*>( key_data.pointer() ), int(key_data.size()), pub_key_as_BN.get())) {
            throw std::runtime_error("ECDSAPublicKey::convertToECPOINT(...): BN_bin2bn(...) failed.");
        }

        UniqPtr<const EC_GROUP> group( EC_GROUP_new_by_curve_name( ECDSACurveType::nid ) );
        UniqPtr<EC_POINT> public_key {EC_POINT_new( group.get())};
        context::Session<BN_CTX> session;

        if( !EC_POINT_bn2point(group.get(), pub_key_as_BN.get(), public_key.get(), session.context().get()) ) {
            throw std::runtime_error("ECDSAPublicKey::convertToECPOINT(...): EC_POINT_bn2point(...) failed.");
        }

        return public_key;
    }

    static UniqPtr<EC_KEY> CreateECKEY(const EC_POINT * key_EC_POINT) {
        UniqPtr<EC_KEY> key {EC_KEY_new_by_curve_name(ECDSACurveType::nid)}; 
        if (!EC_KEY_set_public_key(key.get(), key_EC_POINT)) {
            throw std::runtime_error("ECDSAPublicKey::CreateECKEY(...): EC_KEY_set_public_key(...) failed.")            ;
        }
        return key;
    }

public:

    ECDSAPublicKey(
          ShrdPtr<EC_POINT> public_key,
          const EC_GROUP *group,
          const context::Session<BN_CTX>& session
          ) 
        : key_EC_POINT_ {public_key}
        , key_EC_KEY_ {CreateECKEY(public_key.get())}
        , key_binary_ {convert(public_key.get(), group, session)} {
    }

    ECDSAPublicKey(const byte_array::ConstByteArray& key_data)
        : key_EC_POINT_ {convert(key_data)}
        , key_EC_KEY_ {CreateECKEY(key_EC_POINT_)}
        , key_binary_ {key_data} {
    }

    ShrdPtr<const EC_POINT> keyAsEC_POINT() const {
        return key_EC_POINT_;
    }

    ShrdPtr<const EC_KEY> Key() const {
        return key_EC_KEY_;
    }

    const byte_array::ConstByteArray& keyAsBin() const {
        return key_binary_;
    }

    //static std::shared_ptr<ECDSAPrivateKey> fromData(const byte_array::ConstByteArray& key_data) {
    //}
};

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_PUBLIC_KEY_HPP

