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

#include "crypto/md5.hpp"

#include <openssl/md5.h>

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {

static_assert(MD5::size_in_bytes == MD5_DIGEST_LENGTH, "Incorrect value of MD5::size_in_bytes");

bool MD5::ResetHasher()
{
  return impl_.reset();
}

bool MD5::UpdateHasher(uint8_t const *data_to_hash, std::size_t const size)
{
  return impl_.update(data_to_hash, size);
}

bool MD5::FinalHasher(uint8_t *hash, std::size_t const size)
{
  return impl_.final(hash, size);
}

}  // namespace crypto
}  // namespace fetch
