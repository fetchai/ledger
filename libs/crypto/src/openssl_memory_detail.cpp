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

#include "crypto/openssl_memory_detail.hpp"

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {
namespace detail {

template <>
const FreeFunctionPtr<BN_CTX> DeleterPrimitive<BN_CTX>::function = &BN_CTX_free;

template <>
const FreeFunctionPtr<EC_KEY> DeleterPrimitive<EC_KEY>::function = &EC_KEY_free;

template <>
const FreeFunctionPtr<BIGNUM> DeleterPrimitive<BIGNUM>::function = &BN_free;
template <>
const FreeFunctionPtr<BIGNUM> DeleterPrimitive<BIGNUM, eDeleteStrategy::clearing>::function =
    &BN_clear_free;

template <>
const FreeFunctionPtr<EC_POINT> DeleterPrimitive<EC_POINT>::function = &EC_POINT_free;
template <>
const FreeFunctionPtr<EC_POINT> DeleterPrimitive<EC_POINT, eDeleteStrategy::clearing>::function =
    &EC_POINT_clear_free;

template <>
const FreeFunctionPtr<EC_GROUP> DeleterPrimitive<EC_GROUP>::function = &EC_GROUP_free;
template <>
const FreeFunctionPtr<EC_GROUP> DeleterPrimitive<EC_GROUP, eDeleteStrategy::clearing>::function =
    &EC_GROUP_clear_free;

template <>
const FreeFunctionPtr<ECDSA_SIG> DeleterPrimitive<ECDSA_SIG>::function = &ECDSA_SIG_free;

}  // namespace detail
}  // namespace memory
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
