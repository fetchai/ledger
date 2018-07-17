#ifndef CRYPTO_OPENSSL_COMMON_HPP
#define CRYPTO_OPENSSL_COMMON_HPP

#include "openssl/obj_mac.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

template <const int P_ECDSA_Curve_NID = NID_secp256k1>
struct ECDSACurve
{
    static const int nid = P_ECDSA_Curve_NID;
    static const std::size_t privateKeySize;

    //static void print() {
    //    std::cout << "ECDSACurve<" << nid << ">::privateKeySize = " << privateKeySize << std::endl;
    //}
};

template <>
const std::size_t ECDSACurve<NID_secp256k1>::privateKeySize = 32;

} //* namespace

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_COMMON_HPP

