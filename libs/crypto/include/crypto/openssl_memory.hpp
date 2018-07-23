#ifndef CRYPTO_OPENSSL_MEMORY_HPP
#define CRYPTO_OPENSSL_MEMORY_HPP

#include "crypto/openssl_memory_detail.hpp"
#include <memory>

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {

template <typename T
        , eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical
        , typename T_Deleter = detail::OpenSSLDeleter<T, P_DeleteStrategy>>
using ossl_unique_ptr = std::unique_ptr<T, T_Deleter>;

template <typename T
        , eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical
        , typename T_Deleter = detail::OpenSSLDeleter<T, P_DeleteStrategy>>
class ossl_shared_ptr : public std::shared_ptr<T>
{
public:
    using Base = std::shared_ptr<T>;
    using Deleter = T_Deleter;
    static constexpr eDeleteStrategy deleteStrategy = P_DeleteStrategy;

    using Base::Base;
    using Base::reset;

    template< class Y >
    explicit ossl_shared_ptr( Y* ptr = nullptr) : Base(ptr, Deleter()) {
    }

    template< class Y >
    void reset( Y* ptr ) {
        Base::reset(ptr, Deleter());
    }
};

} //* memory namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_MEMORY_HPP

