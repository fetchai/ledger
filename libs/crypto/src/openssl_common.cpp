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

#include "crypto/openssl_common.hpp"

namespace fetch {
namespace crypto {
namespace openssl {

template <>
char const *const ECDSACurve<NID_secp256k1>::sn = SN_secp256k1;
template <>
std::size_t const ECDSACurve<NID_secp256k1>::privateKeySize = 32;
template <>
std::size_t const ECDSACurve<NID_secp256k1>::publicKeySize = 64;
template <>
std::size_t const ECDSACurve<NID_secp256k1>::signatureSize = 64;

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
