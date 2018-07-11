#ifndef CRYPTO_OPENSSL_MEMORY_HPP
#define CRYPTO_OPENSSL_MEMORY_HPP

#include "crypto/openssl_memory_detail.hpp"
#include <memory>

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {

    template <typename T
            , const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical
            , typename T_Deleter = detail::OpenSSLDeleter<T, P_DeleteStrategy>>
    using ossl_unique_ptr = std::unique_ptr<T, T_Deleter>;

} //* memory namespace
} //* openssl namespace
} //* crypto namespace
} //* fetch namespace

#endif //CRYPTO_OPENSSL_MEMORY_HPP

