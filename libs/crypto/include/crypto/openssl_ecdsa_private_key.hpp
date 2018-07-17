#ifndef CRYPTO_OPENSSL_PRIVATE_KEY_HPP
#define CRYPTO_OPENSSL_PRIVATE_KEY_HPP

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/openssl_common.hpp"
#include "crypto/openssl_memory.hpp"
#include "crypto/openssl_context_session.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template<const int P_ECDSA_Curve_NID = NID_secp256k1>
class ECDSAPrivateKey
{
public:
    using eDelStrat = memory::eDeleteStrategy;
    template <typename T
            , const eDelStrat P_DeleteStrategy = eDelStrat::canonical>
    using ShrdPtr = memory::ossl_shared_ptr<T, P_DeleteStrategy>;
    using ECDSACurveType = ECDSACurve<P_ECDSA_Curve_NID>;

private:
    //const byte_array::ConstByteArray _key_data;
    ShrdPtr<EC_KEY> _key;

    static ShrdPtr<BIGNUM, eDelStrat::clearing> keyAsBN(const byte_array::ConstByteArray& key_data) {
        if (ECDSACurveType::privateKeySize != key_data.size()) {
            throw std::runtime_error("Lenght of provided byte array does not correspond to expected lenght for selected elliptic curve");
        }

        ShrdPtr<BIGNUM, eDelStrat::clearing> private_key_as_BN(BN_new());
        BN_bin2bn(key_data.pointer(), int(ECDSACurveType::privateKeySize), private_key_as_BN.get());
        return private_key_as_BN;
    }

    static ShrdPtr<BIGNUM, eDelStrat::clearing> keyAsBN(const std::string& hex_string_key_data) {
        assert(ECDSACurveType::privateKeySize*2 == hex_string_key_data.size());
        BIGNUM *private_key_as_BN = nullptr;
        BN_hex2bn(&private_key_as_BN, hex_string_key_data.c_str());
        return ShrdPtr<BIGNUM, eDelStrat::clearing>(private_key_as_BN);
    }

    ECDSAPrivateKey(ShrdPtr<BIGNUM, eDelStrat::clearing> private_key_as_BN)
        :/* _key_data(key_data)
        ,*/ _key(EC_KEY_new_by_curve_name(NID_secp256k1)) {

        const int res = EC_KEY_set_private_key(_key.get(), private_key_as_BN.get());
        if (!res) {
            throw std::runtime_error("EC_KEY_set_private_key(...) failed.");
        }

        const EC_GROUP *group = EC_KEY_get0_group(_key.get());
        memory::ossl_unique_ptr<EC_POINT> public_key(EC_POINT_new(group));
        context::Session<BN_CTX> session{ShrdPtr<BN_CTX>(BN_CTX_new())};
        
        if (!EC_POINT_mul(group, public_key.get(), private_key_as_BN.get(), NULL, NULL, session.context().get())) {
            throw std::runtime_error("EC_POINT_mul(...) failed.");
        }
        
        if (!EC_KEY_set_public_key(_key.get(), public_key.get())) {
            throw std::runtime_error("EC_KEY_set_public_key(...) failed.");
        }
    }

public:
    ECDSAPrivateKey() = delete;

    ECDSAPrivateKey(const byte_array::ConstByteArray& key_data)
        : ECDSAPrivateKey(ECDSAPrivateKey::keyAsBN(key_data)) {
    }

    ECDSAPrivateKey(const std::string& hex_string_key_data)
        : ECDSAPrivateKey(ECDSAPrivateKey::keyAsBN(hex_string_key_data)) {
    }

    ShrdPtr<EC_KEY> key() const {
        return _key;
    }

    //static std::shared_ptr<ECDSAPrivateKey> fromData(const byte_array::ConstByteArray& key_data) {
    //}
};

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_PRIVATE_KEY_HPP

