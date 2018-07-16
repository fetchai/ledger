#ifndef CRYPTO_OPENSSL_PRIVATE_KEY_HPP
#define CRYPTO_OPENSSL_PRIVATE_KEY_HPP

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/openssl_common.hpp"
#include "crypto/openssl_memory.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template<const int P_ECDSA_Curve_NID = NID_secp256k1>
class ECDSAPrivateKey
{
    //const byte_array::ConstByteArray _key_data;
    memory::ossl_shared_ptr<EC_KEY> _key;

public:
    using ECDSACurveType = ECDSACurve<P_ECDSA_Curve_NID>;
    
    ECDSAPrivateKey() = delete;

    ECDSAPrivateKey(const byte_array::ConstByteArray& key_data)
        :/* _key_data(key_data)
        ,*/ _key(EC_KEY_new_by_curve_name(NID_secp256k1)) {

        assert(ECDSACurveType::privateKeySize == _key_data.size());

        memory::ossl_shared_ptr<BIGNUM, memory::eDeleteStrategy::clearing> private_key_as_BN(BN_new());
        BN_bin2bn(key_data.pointer(), int(ECDSACurveType::privateKeySize), private_key_as_BN.get());
        EC_KEY_set_private_key(_key.get(), private_key_as_BN.get());
    }

    memory::ossl_shared_ptr<const EC_KEY> get() const {
        return _key;
    }

    //static std::shared_ptr<ECDSAPrivateKey> fromData(const byte_array::ConstByteArray& key_data) {
    //}
};

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_PRIVATE_KEY_HPP

