#pragma once
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

#include "crypto/openssl_digests.hpp"
#include "crypto/stream_hasher.hpp"

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {

class MD5 : public internal::StreamHasher<MD5>
{
public:
  using BaseType = internal::StreamHasher<MD5>;

  using BaseType::Final;
  using BaseType::Reset;
  using BaseType::Update;

  static constexpr std::size_t size_in_bytes = 16u;

  MD5()  = default;
  ~MD5() = default;

private:
  bool ResetHasher();
  bool UpdateHasher(uint8_t const *data_to_hash, std::size_t size);
  bool FinalHasher(uint8_t *hash, std::size_t size);

  internal::OpenSslDigestContext impl_{internal::OpenSslDigestType::MD5};

  friend BaseType;
};

}  // namespace crypto
}  // namespace fetch
