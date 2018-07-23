#include "crypto/openssl_common.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template<> const std::size_t ECDSACurve<NID_secp256k1>::privateKeySize = 32;

} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

