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

        template<>
        struct DeleterPrimitive<BIGNUM> {
            static constexpr FreeFunctionPtr<BIGNUM> function = &BN_free;
        };

        template<>
        struct DeleterPrimitive<BIGNUM, eDeleteStrategy::clearing> {
            static constexpr FreeFunctionPtr<BIGNUM> function = &BN_clear_free;
        };

        template<>
        struct DeleterPrimitive<BN_CTX> {
            static constexpr FreeFunctionPtr<BN_CTX> function = &BN_CTX_free;
        };

        template<>
        struct DeleterPrimitive<EC_POINT> {
            static constexpr FreeFunctionPtr<EC_POINT> function = &EC_POINT_free;
        };

        template<>
        struct DeleterPrimitive<EC_POINT, eDeleteStrategy::clearing> {
            static constexpr FreeFunctionPtr<EC_POINT> function = &EC_POINT_clear_free;
        };

        template<>
        struct DeleterPrimitive<EC_KEY> {
            static constexpr FreeFunctionPtr<EC_KEY> function = &EC_KEY_free;
        };

        template<>
        struct DeleterPrimitive<EC_GROUP> {
            static constexpr FreeFunctionPtr<EC_GROUP> function = &EC_GROUP_free;
        };

        template<>
        struct DeleterPrimitive<EC_GROUP, eDeleteStrategy::clearing> {
            static constexpr FreeFunctionPtr<EC_GROUP> function = &EC_GROUP_clear_free;
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

