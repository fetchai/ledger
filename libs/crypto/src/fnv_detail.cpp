#include "crypto/fnv_detail.hpp"

namespace fetch {
namespace crypto {
namespace detail {

template<> FNVConfig<uint32_t>::number_type const FNVConfig<uint32_t>::prime = (1ull << 24) + (1ull << 8) + 0x93ull;
template<> FNVConfig<uint32_t>::number_type const FNVConfig<uint32_t>::offset = 0x811c9dc5;

template<> FNVConfig<uint64_t>::number_type const FNVConfig<uint64_t>::prime = (1ull << 40) + (1ull << 8) + 0xb3ull;
template<> FNVConfig<uint64_t>::number_type const FNVConfig<uint64_t>::offset = 0xcbf29ce484222325;

} // namespace detail
} // namespace crypto
} // namespace fetch
