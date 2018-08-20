#pragma once
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

#include "core/byte_array/byte_array.hpp"
#include "crypto/stream_hasher.hpp"

#include <openssl/sha.h>
#include <stdexcept>

namespace fetch {
namespace crypto {

class SHA256 : public StreamHasher
{
public:
  using byte_array_type = typename StreamHasher::byte_array_type;

  void Reset() override
  {
    digest_.Resize(0);
    if (!SHA256_Init(&data_)) throw std::runtime_error("could not intialialise SHA256.");
  }

  bool Update(byte_array_type const &s) override { return Update(s.pointer(), s.size()); }

  void Final() override
  {
    digest_.Resize(SHA256_DIGEST_LENGTH);
    Final(reinterpret_cast<uint8_t *>(digest_.pointer()));
  }

  bool Update(uint8_t const *p, std::size_t const &size) override
  {
    if (!SHA256_Update(&data_, p, size)) return false;
    return true;
  }

  void Final(uint8_t *p) override
  {
    if (!SHA256_Final(p, &data_)) throw std::runtime_error("could not finalize SHA256.");
  }

  byte_array_type digest() override
  {
    assert(digest_.size() == SHA256_DIGEST_LENGTH);
    return digest_;
  }

private:
  byte_array_type digest_;
  SHA256_CTX      data_;
};
}  // namespace crypto
}  // namespace fetch
