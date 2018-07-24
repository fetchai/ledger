#include "crypto/openssl_common.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template<> const char * const ECDSACurve<NID_secp256k1>::sn = SN_secp256k1;
template<> const std::size_t ECDSACurve<NID_secp256k1>::privateKeySize = 32;
template<> const std::size_t ECDSACurve<NID_secp256k1>::publicKeySize = 64;
template<> const std::size_t ECDSACurve<NID_secp256k1>::signatureSize = 64;

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

