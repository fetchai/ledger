#ifndef CRYPTO_OPENSSL_MEMORY_DETAIL_HPP
#define CRYPTO_OPENSSL_MEMORY_DETAIL_HPP

#include <openssl/bn.h>
#include <openssl/ec.h>


namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {

    enum eDeleteStrategy : int {
        canonical, //* XXX_free(...)
        clearing   //* XXX_clear_free(...)
    };

    namespace detail {

        template <typename T>
        using FreeFunctionPtr = void(*)(T*);

        template <typename T
                , const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical>
        struct DeleterPrimitive;

        template <>
        struct DeleterPrimitive<BN_CTX> {
            static const FreeFunctionPtr<BN_CTX> function;
        };

        template <>
        struct DeleterPrimitive<EC_KEY> {
            static const FreeFunctionPtr<EC_KEY> function;
        };

        template <>
        struct DeleterPrimitive<BIGNUM> {
            static const FreeFunctionPtr<BIGNUM> function;
        };

        template <>
        struct DeleterPrimitive<BIGNUM, eDeleteStrategy::clearing> {
            static const FreeFunctionPtr<BIGNUM> function;
        };

        template <>
        struct DeleterPrimitive<EC_POINT> {
            static const FreeFunctionPtr<EC_POINT> function;
        };

        template <>
        struct DeleterPrimitive<EC_POINT, eDeleteStrategy::clearing> {
            static const FreeFunctionPtr<EC_POINT> function;
        };

        template <>
        struct DeleterPrimitive<EC_GROUP> {
            static const FreeFunctionPtr<EC_GROUP> function;
        };

        template <>
        struct DeleterPrimitive<EC_GROUP, eDeleteStrategy::clearing> {
            static const FreeFunctionPtr<EC_GROUP> function;
        };

        template <typename T
                 , const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical
                 , typename T_DeleterPrimitive = detail::DeleterPrimitive<T, P_DeleteStrategy>>
        struct OpenSSLDeleter {
            using DeleterPrimitive = T_DeleterPrimitive;

            constexpr OpenSSLDeleter() noexcept = default;

            void operator() (T* ptr) const {
                (*DeleterPrimitive::function)(ptr);
            }
        };
    }

} //* memory namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_MEMORY_DETAIL_HPP

