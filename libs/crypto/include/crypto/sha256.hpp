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

namespace fetch {
namespace crypto {

class SHA256LL : public virtual StreamHasherLowLevel
{
protected:
  static const std::size_t hash_size;

public:
  std::size_t hashSize() const override;
  void        Reset() override;
  bool        Update(uint8_t const *data_to_hash, std::size_t const &size) override;
  void        Final(uint8_t *hash, std::size_t const &size) override;

private:
  SHA256_CTX context_;
};

class SHA256 : public SHA256LL, public virtual StreamHasher
{
public:
  using base_type       = SHA256LL;
  using byte_array_type = typename StreamHasher::byte_array_type;

  using SHA256LL::Update;
  using SHA256LL::Final;

  void            Reset() override;
  bool            Update(byte_array_type const &s) override;
  void            Final() override;
  byte_array_type digest() const override;

private:
  byte_array_type digest_;
};

}  // namespace crypto
}  // namespace fetch
