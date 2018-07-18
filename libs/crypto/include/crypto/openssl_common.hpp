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
    static const int nid = P_ECDSA_Curve_NID;
    static const std::size_t privateKeySize;
};

template<> const std::size_t ECDSACurve<NID_secp256k1>::privateKeySize;

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_COMMON_HPP

