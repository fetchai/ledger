//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "crypto/fnv_detail.hpp"

namespace fetch {
namespace crypto {
namespace detail {

template <>
FNVConfig<uint32_t>::number_type const FNVConfig<uint32_t>::prime = (1ull << 24) + (1ull << 8) +
                                                                    0x93ull;
template <>
FNVConfig<uint32_t>::number_type const FNVConfig<uint32_t>::offset = 0x811c9dc5;

template <>
FNVConfig<uint64_t>::number_type const FNVConfig<uint64_t>::prime   =  (1ull << 40) + (1ull << 8) +
                                                                    0xb3ull;
template <>
FNVConfig<uint64_t>::number_type const FNVConfig<uint64_t>::offset = 0xcbf29ce484222325;

}  // namespace detail
}  // namespace crypto
}  // namespace fetch
