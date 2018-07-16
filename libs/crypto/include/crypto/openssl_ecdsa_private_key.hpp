#ifndef CRYPTO_OPENSSL_PRIVATE_KEY_HPP
#define CRYPTO_OPENSSL_PRIVATE_KEY_HPP

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/openssl_common.hpp"
#include "crypto/openssl_memory.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template<const int P_ECDSA_Curve_NID = NID_secp256k1
       , template<typename T, const memory::eDeleteStrategy P_DeleteStrategy = memory::eDeleteStrategy::canonical, typename T_Deleter=memory::detail::OpenSSLDeleter<T, P_DeleteStrategy>> class T_OSSLSharedPtr = memory::ossl_shared_ptr>
class ECDSAPrivateKey
{
public:
    template<typename T, const memory::eDeleteStrategy P_DeleteStrategy = memory::eDeleteStrategy::canonical> 
    using OSSLSharedPtr = T_OSSLSharedPtr<T, P_DeleteStrategy>;

private:
    const byte_array::ConstByteArray _key_data;
    OSSLSharedPtr<EC_KEY> _key;

public:
    using ECDSACurveType = ECDSACurve<P_ECDSA_Curve_NID>;
    
    ECDSAPrivateKey() = delete;

    ECDSAPrivateKey(const byte_array::ConstByteArray& key_data)
        : _key_data(key_data)
        , _key(EC_KEY_new_by_curve_name(NID_secp256k1)) {

        assert(ECDSACurveType::privateKeySize == _key_data.size());

        OSSLSharedPtr<BIGNUM> private_key_as_BN(BN_new());
        BN_bin2bn(_key_data.pointer(), int(ECDSACurveType::privateKeySize), private_key_as_BN.get());
        EC_KEY_set_private_key(_key.get(), private_key_as_BN.get());
    }

    OSSLSharedPtr<const EC_KEY> get() const {
        return _key;
    }

    //static std::shared_ptr<ECDSAPrivateKey> fromData(const byte_array::ConstByteArray& key_data) {
    //}
};

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_PRIVATE_KEY_HPP

