#ifndef CRYPTO_OPENSSL_MEMORY_HPP
#define CRYPTO_OPENSSL_MEMORY_HPP
#include <crypto/hash.hpp>
#include <core/assert.hpp>
#include <crypto/prover.hpp>
#include <crypto/sha256.hpp>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#include <memory>
#include <functional>

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
        struct Deleter;

        template<>
        struct Deleter<BIGNUM> {
            static constexpr FreeFunctionPtr<BIGNUM> function = &BN_free;
        };

        template<>
        struct Deleter<BIGNUM, eDeleteStrategy::clearing> {
            static constexpr FreeFunctionPtr<BIGNUM> function = &BN_clear_free;
        };

        template<>
        struct Deleter<BN_CTX> {
            static constexpr FreeFunctionPtr<BN_CTX> function = &BN_CTX_free;
        };

        template<>
        struct Deleter<EC_POINT> {
            static constexpr FreeFunctionPtr<EC_POINT> function = &EC_POINT_free;
        };

        template<>
        struct Deleter<EC_POINT, eDeleteStrategy::clearing> {
            static constexpr FreeFunctionPtr<EC_POINT> function = &EC_POINT_clear_free;
        };

        template<>
        struct Deleter<EC_KEY> {
            static constexpr FreeFunctionPtr<EC_KEY> function = &EC_KEY_free;
        };

        template<>
        struct Deleter<EC_GROUP> {
            static constexpr FreeFunctionPtr<EC_GROUP> function = &EC_GROUP_free;
        };

        template<>
        struct Deleter<EC_GROUP, eDeleteStrategy::clearing> {
            static constexpr FreeFunctionPtr<EC_GROUP> function = &EC_GROUP_clear_free;
        };
    }

    template <typename T
             , const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical
             , typename T_Deleter = detail::Deleter<T, P_DeleteStrategy>>
    struct OpenSSLDeleter {
        using Deleter = T_Deleter;

        constexpr OpenSSLDeleter() noexcept = default;

        void operator() (T* ptr) const {
            (*Deleter::function)(ptr);
        }
    };

    template <typename T
            , const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical
            , typename T_Deleter = detail::Deleter<T, P_DeleteStrategy>>
    using ossl_unique_ptr = std::unique_ptr<T, OpenSSLDeleter<T, P_DeleteStrategy, T_Deleter>>;

} //* memory namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace
#endif //CRYPTO_OPENSSL_MEMORY_HPP

