#include "crypto/ecdsa.hpp"

namespace fetch {
namespace crypto {

const std::size_t ECDSASigner::PRIVATE_KEY_SIZE = ECDSASigner::PrivateKey::ecdsa_curve_type::privateKeySize;

} // namespace crypto
} // namespace fetch

