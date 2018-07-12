#ifndef CRYPTO_OPENSSL_MEMORY_DETAIL_HPP
#define CRYPTO_OPENSSL_MEMORY_DETAIL_HPP

#include <openssl/bn.h>
#include <openssl/ec.h>

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

        namespace {
            template <typename T>
            using FreeFunctionPtr = void(*)(T*);

            template <typename T
                    , const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical>
            struct DeleterPrimitive
            {
                static const FreeFunctionPtr<T> function;
            };

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

        }

        template <typename T
                 , const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical
                 , typename T_DeleterPrimitive = detail::DeleterPrimitive<typename std::remove_const<T>::type, P_DeleteStrategy>>
        struct OpenSSLDeleter {
            using DeleterPrimitive = T_DeleterPrimitive;

            constexpr OpenSSLDeleter() noexcept = default;

            void operator() (T* ptr) const {
                (*DeleterPrimitive::function)(const_cast<typename std::remove_const<T>::type *>(ptr));
            }
        };
    }

} //* memory namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_MEMORY_DETAIL_HPP

