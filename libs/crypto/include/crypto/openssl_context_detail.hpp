#ifndef CRYPTO_OPENSSL_CONTEXT_DETAIL_HPP
#define CRYPTO_OPENSSL_CONTEXT_DETAIL_HPP

#include <openssl/bn.h>

namespace fetch {
namespace crypto {
namespace openssl {
namespace context {
namespace detail {

namespace {

    template <typename T>
    using FunctionPtr = void(*)(T*);

    template <typename T>
    struct SessionPrimitive
    {
        static const FunctionPtr<T> start;
        static const FunctionPtr<T> end;
    };

    template<>
    const FunctionPtr<BN_CTX> SessionPrimitive<BN_CTX>::start = &BN_CTX_start;
    template<>
    const FunctionPtr<BN_CTX> SessionPrimitive<BN_CTX>::end = &BN_CTX_end;

} //* namespace

} //* namespace detail
} //* context namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_CONTEXT_DETAIL_HPP

