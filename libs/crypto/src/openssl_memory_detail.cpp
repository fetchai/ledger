#include "crypto/openssl_memory_detail.hpp"

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {
namespace detail {

template<>
const FreeFunctionPtr<BN_CTX> DeleterPrimitive<BN_CTX>::function = &BN_CTX_free;

template<>
const FreeFunctionPtr<EC_KEY> DeleterPrimitive<EC_KEY>::function = &EC_KEY_free;

template<>
const FreeFunctionPtr<BIGNUM> DeleterPrimitive<BIGNUM>::function = &BN_free;
template<>
const FreeFunctionPtr<BIGNUM> DeleterPrimitive<BIGNUM, eDeleteStrategy::clearing>::function = &BN_clear_free;

template<>
const FreeFunctionPtr<EC_POINT> DeleterPrimitive<EC_POINT>::function = &EC_POINT_free;
template<>
const FreeFunctionPtr<EC_POINT> DeleterPrimitive<EC_POINT, eDeleteStrategy::clearing>::function = &EC_POINT_clear_free;

template<>
const FreeFunctionPtr<EC_GROUP> DeleterPrimitive<EC_GROUP>::function = &EC_GROUP_free;
template<>
const FreeFunctionPtr<EC_GROUP> DeleterPrimitive<EC_GROUP, eDeleteStrategy::clearing>::function = &EC_GROUP_clear_free;

} //* detail namespace
} //* memory namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

