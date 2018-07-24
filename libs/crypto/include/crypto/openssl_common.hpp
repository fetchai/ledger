#ifndef CRYPTO_OPENSSL_COMMON_HPP
#define CRYPTO_OPENSSL_COMMON_HPP

#include "openssl/obj_mac.h"

#include <cstddef>

namespace fetch {
namespace crypto {
namespace openssl {

template <int P_ECDSA_Curve_NID = NID_secp256k1>
struct ECDSACurve
{
    static const int nid;
    static const char * const sn;
    static const std::size_t privateKeySize;
    static const std::size_t publicKeySize;
    static const std::size_t signatureSize;
};

template <int P_ECDSA_Curve_NID>
const int ECDSACurve<P_ECDSA_Curve_NID>::nid = P_ECDSA_Curve_NID;

template<> const char * const ECDSACurve<NID_secp256k1>::sn;
template<> const std::size_t ECDSACurve<NID_secp256k1>::privateKeySize;
template<> const std::size_t ECDSACurve<NID_secp256k1>::publicKeySize;
template<> const std::size_t ECDSACurve<NID_secp256k1>::signatureSize;

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_COMMON_HPP

