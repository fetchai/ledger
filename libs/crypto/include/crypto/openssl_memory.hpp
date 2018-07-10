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

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {

    enum eDeleteStrategy : int {
        canonical, //* XXX_free(...)
        clearing   //* XXX_clear_free(...)
    };

    namespace {

        template <typename T, const eDeleteStrategy deleteStrategy = eDeleteStrategy::canonical>
        using FunctionType = void(*)(T*);

        template <typename T, const eDeleteStrategy deleteStrategy = eDeleteStrategy::canonical>
        struct DeleterFunction {
            static constexpr FunctionType<T, deleteStrategy> default_fnc();
            static constexpr FunctionType<T, deleteStrategy> deleterFunction = default_fnc();
        };

        template<>
        struct DeleterFunction<BIGNUM> {
            static constexpr FunctionType<BIGNUM> deleterFunction = &BN_free;
        };

        template<>
        struct DeleterFunction<BIGNUM, eDeleteStrategy::clearing> {
            static constexpr FunctionType<BIGNUM, eDeleteStrategy::clearing> deleterFunction = &BN_clear_free;
        };

        template<>
        struct DeleterFunction<BN_CTX> {
            static constexpr FunctionType<BN_CTX> deleterFunction = &BN_CTX_free;
        };

        template<>
        struct DeleterFunction<EC_POINT> {
            static constexpr FunctionType<EC_POINT> deleterFunction = &EC_POINT_free;
        };

        template<>
        struct DeleterFunction<EC_POINT, eDeleteStrategy::clearing> {
            static constexpr FunctionType<EC_POINT, eDeleteStrategy::clearing> deleterFunction = &EC_POINT_clear_free;
        };

        template<>
        struct DeleterFunction<EC_KEY> {
            static constexpr FunctionType<EC_KEY> deleterFunction = &EC_KEY_free;
        };

        template<>
        struct DeleterFunction<EC_GROUP> {
            static constexpr FunctionType<EC_GROUP> deleterFunction = &EC_GROUP_free;
        };

        template<>
        struct DeleterFunction<EC_GROUP, eDeleteStrategy::clearing> {
            static constexpr FunctionType<EC_GROUP, eDeleteStrategy::clearing> deleterFunction = &EC_GROUP_clear_free;
        };
    }

    template <typename T
             , const eDeleteStrategy deleteStrategy
             , typename T_DeleterFunction = DeleterFunction<T, deleteStrategy>>
    struct OpenSSLDeleter {
        using DeleterFunction = T_DeleterFunction;

        constexpr OpenSSLDeleter() noexcept = default;

        void operator() (T* ptr) const {
            (*DeleterFunction::deleterFunction)(ptr);
        }
    };

    template <typename T
            , const eDeleteStrategy deleteStrategy
            , typename T_DeleterFunction = DeleterFunction<T, deleteStrategy>>
    using ossl_unique_ptr = std::unique_ptr<T, OpenSSLDeleter<T, deleteStrategy, T_DeleterFunction>>;

    //template <>
    //void OpenSSLDeleter<>::operator()<BIGNUM> (BIGNUM* ptr) const {
    //    BN_free(ptr);
    //}

    //template <> template <>
    //void OpenSSLDeleter<eDeleteStrategy::clearing>::operator()<BIGNUM> (BIGNUM* ptr) const {
    //    BN_clear_free(ptr);
    //}

    //template <> template <>
    //void OpenSSLDeleter<>::operator()<BN_CTX> (BN_CTX* ptr) const {
    //    BN_CTX_free(ptr);
    //}

    //template <> template <>
    //void OpenSSLDeleter<>::operator()<EC_POINT> (EC_POINT* ptr) const {
    //    EC_POINT_free(ptr);
    //}

    //template <> template <>
    //void OpenSSLDeleter<eDeleteStrategy::clearing>::operator()<EC_POINT> (EC_POINT* ptr) const {
    //    EC_POINT_clear_free(ptr);
    //}

    //template <> template <>
    //void OpenSSLDeleter<>::operator()<EC_KEY> (EC_KEY* ptr) const {
    //    EC_KEY_free(ptr);
    //}

    //template <> template <>
    //void OpenSSLDeleter<>::operator()<EC_GROUP> (EC_GROUP* ptr) const {
    //    EC_GROUP_free(ptr);
    //}

    //template <> template <>
    //void OpenSSLDeleter<eDeleteStrategy::clearing>::operator()<EC_GROUP> (EC_GROUP* ptr) const {
    //    EC_GROUP_clear_free(ptr);
    //}

    //template <typename T, const eDeleteStrategy deleteStrategy = eDeleteStrategy::canonical>
    //using ossl_unique_ptr = std::unique_ptr<T, OpenSSLDeleter<deleteStrategy>>;

    //namespace appr2 {

    //    template <typename T>
    //    using deleter_type = void(*)(T* ptr);

    //    namespace {
    //        template <typename T, constexpr deleter_type deleter>
    //        struct OpesSSLDeleter {
    //            void operator() (T* ptr) {
    //                deleter(ptr);
    //            }
    //        };

    //        template <typename T>
    //        struct OpesSSLDeleter<T, nullptr> {
    //            void operator() (T* ptr);
    //        };

    //        using OpesSSLDeleter
    //    } //* annonymous namespace 
    //} //* appr2 namespace

} //* memory namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace
#endif //CRYPTO_OPENSSL_MEMORY_HPP

