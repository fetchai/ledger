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

#include "crypto/sha256.hpp"

#include <openssl/sha.h>

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {

static_assert(SHA256::size_in_bytes == SHA256_DIGEST_LENGTH,
              "Incorrect value of SHA256::size_in_bytes");

namespace {

bool ResetContext(SHA256_CTX &context)
{
  return static_cast<bool>(SHA256_Init(&context));
}

}  // namespace

namespace internal {

class Sha256Internals
{
public:
  SHA256_CTX ctx;
};

}  // namespace internal

SHA256::SHA256()
  : impl_(new internal::Sha256Internals)
{
  ResetContext(impl_->ctx);
}

SHA256::~SHA256()
{
  delete impl_;
}

bool SHA256::ResetHasher()
{
  return ResetContext(impl_->ctx);
}

bool SHA256::UpdateHasher(uint8_t const *data_to_hash, std::size_t const &size)
{
  return SHA256_Update(&(impl_->ctx), data_to_hash, size) != 0;
}

bool SHA256::FinalHasher(uint8_t *hash, std::size_t const &)
{
  auto const success = SHA256_Final(hash, &(impl_->ctx));

  return static_cast<bool>(success);
}

}  // namespace crypto
}  // namespace fetch
