#ifndef CRYPTO_OPENSSL_PRIVATE_KEY_HPP
#define CRYPTO_OPENSSL_PRIVATE_KEY_HPP

//#include "core/byte_array/const_byte_array.hpp"
//#include "crypto/openssl_common.hpp"
//#include "crypto/openssl_memory.hpp"
//#include "crypto/openssl_context_session.hpp"
#include "crypto/openssl_ecdsa_public_key.hpp"


namespace fetch {
namespace crypto {
namespace openssl {

template<int P_ECDSA_Curve_NID = NID_secp256k1
       , point_conversion_form_t P_ConversionForm = POINT_CONVERSION_UNCOMPRESSED>
class ECDSAPrivateKey
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
    using PublicKeyType = ECDSAPublicKey<P_ECDSA_Curve_NID, P_ConversionForm>;

    static constexpr point_conversion_form_t conversionForm = P_ConversionForm;

private:
    //TODO: Keep key encrypted
    const ShrdPtr<EC_KEY> _private_key;
    const PublicKeyType _public_key;
    //const byte_array::ConstByteArray _key_data;

    static UniqPtr<BIGNUM, eDelStrat::clearing> keyAsBN(const byte_array::ConstByteArray& key_data) {
        if (ECDSACurveType::privateKeySize != key_data.size()) {
            throw std::runtime_error("ECDSAPrivateKey::keyAsBN(const byte_array::ConstByteArray&): Lenght of provided byte array does not correspond to expected lenght for selected elliptic curve");
        }
        UniqPtr<BIGNUM, eDelStrat::clearing> private_key_as_BN(BN_new());
        BN_bin2bn(key_data.pointer(), int(ECDSACurveType::privateKeySize), private_key_as_BN.get());
        return private_key_as_BN;
    }

    static UniqPtr<BIGNUM, eDelStrat::clearing> keyAsBN(const std::string& hex_string_key_data) {
        if( ECDSACurveType::privateKeySize*2 != hex_string_key_data.size() ) {
            throw std::runtime_error("ECDSAPrivateKey::keyAsBN(const std::string&): Lenght of provided byte array does not correspond to expected lenght for priv key for selected elliptic curve");
        }
        BIGNUM *private_key_as_BN = nullptr;
        BN_hex2bn(&private_key_as_BN, hex_string_key_data.c_str());
        return UniqPtr<BIGNUM, eDelStrat::clearing>(private_key_as_BN);
    }

    static UniqPtr<EC_KEY> keyAsECKEY( const BIGNUM* private_key_as_BN ) {
        UniqPtr<EC_KEY> priv_key( EC_KEY_new_by_curve_name( ECDSACurveType::nid ) );
        if( !EC_KEY_set_private_key( priv_key.get(), private_key_as_BN ) ) {
            throw std::runtime_error("ECDSAPrivateKey::keyAsECKEY(...): EC_KEY_set_private_key(...) failed.");
        }
        return priv_key;
    }

    static PublicKeyType derivePublicKey(const BIGNUM *private_key_as_BN, EC_KEY *private_key) {
        const EC_GROUP *group = EC_KEY_get0_group(private_key);
        ShrdPtr<EC_POINT> public_key {EC_POINT_new(group)};
        context::Session<BN_CTX> session;

        if( !EC_POINT_mul(group, public_key.get(), private_key_as_BN, NULL, NULL, session.context().get())) {
            throw std::runtime_error("ECDSAPrivateKey::derivePublicKey(...): EC_POINT_mul(...) failed.");
        }

        if(!EC_KEY_set_public_key(private_key, public_key.get())) {
            throw std::runtime_error("ECDSAPrivateKey::derivePublicKey(...): EC_KEY_set_public_key(...) failed.");
        }

        return PublicKeyType {public_key, group, session};
    }

    static PublicKeyType extractPublicKey(const EC_KEY *private_key) {
        const EC_GROUP *group = EC_KEY_get0_group(private_key);
        const EC_POINT *pub_key_reference = EC_KEY_get0_public_key(private_key);
        ShrdPtr<EC_POINT> public_key {EC_POINT_dup(pub_key_reference, group)};
        if(!public_key) {
            throw std::runtime_error("ECDSAPrivateKey::extractPublicKey(...): EC_POINT_dup(...) failed.");
        }

        return  PublicKeyType {public_key, group, context::Session<BN_CTX>{}};
    }

   static UniqPtr<EC_KEY> GenerateKey() {
        UniqPtr<EC_KEY> key {EC_KEY_new_by_curve_name(ECDSACurveType::nid)};
        if(!key) {
            throw std::runtime_error("ECDSAPrivateKey::GenerateKey(): EC_KEY_new_by_curve_name(...) failed.");
        }

        if(!EC_KEY_generate_key(key.get())) {
            throw std::runtime_error("ECDSAPrivateKey::GenerateKey(): EC_KEY_generate_key(...) failed.");
        }

        ////TODO: Is it really necessary to call check after generate (generate shall be sufficient(shall fail in case of issue))
        //if(!EC_KEY_check_key(key.get())) {
        //    throw std::runtime_error("ECDSAPrivateKey::GenerateKey(): EC_KEY_check_key(...) failed.");
        //}

        return key;
    }

   ECDSAPrivateKey(ShrdPtr<BIGNUM, eDelStrat::clearing> private_key_as_BN)
        : _private_key( keyAsECKEY( private_key_as_BN.get() ) )
        , _public_key( derivePublicKey( private_key_as_BN.get(), _private_key.get() ) ) {
    }

public:

    ECDSAPrivateKey()
        : _private_key(GenerateKey())
        , _public_key(extractPublicKey(_private_key.get())) {
    }

    ECDSAPrivateKey(const byte_array::ConstByteArray& key_data)
        : ECDSAPrivateKey(ECDSAPrivateKey::keyAsBN(key_data)) {
    }

    ECDSAPrivateKey(const std::string& hex_string_key_data)
        : ECDSAPrivateKey(ECDSAPrivateKey::keyAsBN(hex_string_key_data)) {
    }

    ShrdPtr<const EC_KEY> key() const {
        return _private_key;
    }

    const PublicKeyType& publicKey() const {
        return _public_key;
    }
};

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_PRIVATE_KEY_HPP

