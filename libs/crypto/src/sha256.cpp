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

#include "core/macros.hpp"
#include "crypto/sha256.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {

void SHA256::Reset()
{
  auto const success = openssl_hasher_.reset();
  FETCH_UNUSED(success);
  assert(success);
}

bool SHA256::Update(uint8_t const *const data_to_hash, std::size_t const size)
{
  return openssl_hasher_.update(data_to_hash, size);
}

void SHA256::Final(uint8_t *const hash)
{
  auto const success = openssl_hasher_.final(hash);
  FETCH_UNUSED(success);
  assert(success);
}

std::size_t SHA256::HashSizeInBytes() const
{
  auto const size = openssl_hasher_.hash_size();
  assert(size == size_in_bytes);
  return size;
}

}  // namespace crypto
}  // namespace fetch
