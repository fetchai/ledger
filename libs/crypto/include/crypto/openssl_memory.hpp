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

    enum eFreeingType : int {
        canonical, //* canonical XXX_free(...)
        clearing   //* XXX_clear_free(...)
    };

    template <const eFreeingType freeingType = eFreeingType::canonical>
    struct OpenSSLDeleter {
        constexpr OpenSSLDeleter() noexcept = default;

        template <typename T>
        void operator() (T* ptr) const;
    };

    template <> template <>
    void OpenSSLDeleter<>::operator()<BIGNUM> (BIGNUM* ptr) const {
        BN_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<eFreeingType::clearing>::operator()<BIGNUM> (BIGNUM* ptr) const {
        BN_clear_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<>::operator()<BN_CTX> (BN_CTX* ptr) const {
        BN_CTX_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<>::operator()<EC_POINT> (EC_POINT* ptr) const {
        EC_POINT_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<eFreeingType::clearing>::operator()<EC_POINT> (EC_POINT* ptr) const {
        EC_POINT_clear_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<>::operator()<EC_KEY> (EC_KEY* ptr) const {
        EC_KEY_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<>::operator()<EC_GROUP> (EC_GROUP* ptr) const {
        EC_GROUP_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<eFreeingType::clearing>::operator()<EC_GROUP> (EC_GROUP* ptr) const {
        EC_GROUP_clear_free(ptr);
    }

    template <typename T, const eFreeingType freeingType = eFreeingType::canonical>
    using ossl_unique_ptr = std::unique_ptr<T, OpenSSLDeleter<freeingType>>;

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
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace
#endif //CRYPTO_OPENSSL_MEMORY_HPP

