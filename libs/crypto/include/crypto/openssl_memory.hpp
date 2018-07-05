#ifndef CRYPTO_ECDSA_HPP
#define CRYPTO_ECDSA_HPP
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

    template <constexpr eDeleterType freeingType = eFreeingType::canonical>
    struct OpenSSLDeleter {
        template <typename T>
        void operator() (const T* ptr);
    };

    template <> template <>
    void OpenSSLDeleter<>::operator()<BIGNUM> (const BIGNUM* ptr) {
        BN_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<eFreeingType::clearing>::operator()<BIGNUM> (const BIGNUM* ptr) {
        BN_clear_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<>::operator()<BN_CTX> (const BN_CTX* ptr) {
        BN_CTX_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<>::operator()<EC_POINT> (const EC_POINT* ptr) {
        EC_POINT_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<eFreeingType::clearing>::operator()<EC_POINT> (const EC_POINT* ptr) {
        EC_POINT_clear_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<>::operator()<EC_KEY> (const EC_KEY* ptr) {
        EC_KEY_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<>::operator()<EC_GROUP> (const EC_KEY* ptr) {
        EC_GROUP_free(ptr);
    }

    template <> template <>
    void OpenSSLDeleter<eFreeingType::clearing>::operator()<EC_GROUP> (const EC_GROUP* ptr) {
        EC_GROUP_clear_free(ptr);
    }

    template <typename T, constexpr eDeleterType freeingType = eFreeingType::canonical>
    using ossl_unique_ptr = std::unique_ptr<T, OpenSSLDeleter<freeingType>>;

    //namespace appr2 {

    //    template <typename T>
    //    using deleter_type = void(*)(const T* ptr);

    //    namespace {
    //        template <typename T, constexpr deleter_type deleter>
    //        struct OpesSSLDeleter {
    //            void operator() (const T* ptr) {
    //                deleter(ptr);
    //            }
    //        };

    //        template <typename T>
    //        struct OpesSSLDeleter<T, nullptr> {
    //            void operator() (const T* ptr);
    //        };

    //        using OpesSSLDeleter
    //    } //* annonymous namespace 
    //} //* appr2 namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

