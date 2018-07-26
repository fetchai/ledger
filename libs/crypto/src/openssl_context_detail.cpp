#include "crypto/openssl_context_detail.hpp"

namespace fetch {
namespace crypto {
namespace openssl {
namespace context {
namespace detail {

template <>
const FunctionPtr<BN_CTX> SessionPrimitive<BN_CTX>::start = &BN_CTX_start;
template <>
const FunctionPtr<BN_CTX> SessionPrimitive<BN_CTX>::end = &BN_CTX_end;

}  // namespace detail
}  // namespace context
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
