#ifndef CRYPTO_OPENSSL_MEMORY_DETAIL_HPP
#define CRYPTO_OPENSSL_MEMORY_DETAIL_HPP

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>

#include <type_traits>

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
                , eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical>
        struct DeleterPrimitive
        {
            static const FreeFunctionPtr<T> function;
        };

        template<>
        const FreeFunctionPtr<BN_CTX> DeleterPrimitive<BN_CTX>::function;

        template<>
        const FreeFunctionPtr<EC_KEY> DeleterPrimitive<EC_KEY>::function;

        template<>
        const FreeFunctionPtr<BIGNUM> DeleterPrimitive<BIGNUM>::function;
        template<>
        const FreeFunctionPtr<BIGNUM> DeleterPrimitive<BIGNUM, eDeleteStrategy::clearing>::function;

        template<>
        const FreeFunctionPtr<EC_POINT> DeleterPrimitive<EC_POINT>::function;
        template<>
        const FreeFunctionPtr<EC_POINT> DeleterPrimitive<EC_POINT, eDeleteStrategy::clearing>::function;

        template<>
        const FreeFunctionPtr<EC_GROUP> DeleterPrimitive<EC_GROUP>::function;
        template<>
        const FreeFunctionPtr<EC_GROUP> DeleterPrimitive<EC_GROUP, eDeleteStrategy::clearing>::function;

        template<>
        const FreeFunctionPtr<ECDSA_SIG> DeleterPrimitive<ECDSA_SIG>::function;

        template <typename T
                , eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical
                , typename T_DeleterPrimitive = detail::DeleterPrimitive<typename std::remove_const<T>::type, P_DeleteStrategy>>
        struct OpenSSLDeleter {
            using Type = T;
            using DeleterPrimitive = T_DeleterPrimitive;
            static constexpr eDeleteStrategy deleteStrategy = P_DeleteStrategy;

            constexpr OpenSSLDeleter() noexcept = default;

            void operator() (T* ptr) const {
                (*DeleterPrimitive::function)(const_cast<typename std::remove_const<T>::type *>(ptr));
            }
        };

    } //* detail namespace

} //* memory namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_MEMORY_DETAIL_HPP

