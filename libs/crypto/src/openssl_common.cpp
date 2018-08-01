#include "crypto/openssl_common.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template <>
char const *const ECDSACurve<NID_secp256k1>::sn = SN_secp256k1;
template <>
std::size_t const ECDSACurve<NID_secp256k1>::privateKeySize = 32;
template <>
std::size_t const ECDSACurve<NID_secp256k1>::publicKeySize = 64;
template <>
std::size_t const ECDSACurve<NID_secp256k1>::signatureSize = 64;

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
