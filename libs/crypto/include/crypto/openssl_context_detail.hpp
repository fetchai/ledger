#ifndef CRYPTO_OPENSSL_CONTEXT_DETAIL_HPP
#define CRYPTO_OPENSSL_CONTEXT_DETAIL_HPP

#include <openssl/bn.h>

namespace fetch {
namespace crypto {
namespace openssl {
namespace context {
namespace detail {

template <typename T>
using FunctionPtr = void (*)(T *);

// template <typename T>
// struct SessionPrimitive;
//
// template <>
// struct SessionPrimitive<BN_CTX>
//{
//    static const FunctionPtr<BN_CTX> start;
//    static const FunctionPtr<BN_CTX> end;
//};

template <typename T>
struct SessionPrimitive
{
  static const FunctionPtr<T> start;
  static const FunctionPtr<T> end;
};

template <>
const FunctionPtr<BN_CTX> SessionPrimitive<BN_CTX>::start;
template <>
const FunctionPtr<BN_CTX> SessionPrimitive<BN_CTX>::end;

}  // namespace detail
}  // namespace context
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch

#endif  // CRYPTO_OPENSSL_CONTEXT_DETAIL_HPP
