//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include <stdexcept>

namespace fetch {
namespace crypto {

const std::size_t SHA256LL::hash_size{SHA256_DIGEST_LENGTH};

std::size_t SHA256LL::hashSize() const { return hash_size; }

void SHA256LL::Reset()
{
  if (!SHA256_Init(&context_)) throw std::runtime_error("could not intialialise SHA256LL.");
}

bool SHA256LL::Update(uint8_t const *data_to_hash, std::size_t const &size)
{
  if (!SHA256_Update(&context_, data_to_hash, size)) return false;
  return true;
}

void SHA256LL::Final(uint8_t *hash, std::size_t const &size)
{
  if (size < hash_size) throw std::runtime_error("size of input buffer is smaller than hash size.");
  if (!SHA256_Final(hash, &context_)) throw std::runtime_error("could not finalize SHA256.");
}

void SHA256::Reset()
{
  digest_.Resize(0);
  base_type::Reset();
}

bool SHA256::Update(byte_array_type const &s) { return Update(s.pointer(), s.size()); }

void SHA256::Final()
{
  digest_.Resize(hash_size);
  Final(reinterpret_cast<uint8_t *>(digest_.pointer()), hash_size);
}

SHA256::byte_array_type SHA256::digest() const
{
  assert(digest_.size() == hash_size);
  return digest_;
}

}  // namespace crypto
}  // namespace fetch
