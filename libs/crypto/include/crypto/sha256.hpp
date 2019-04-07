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

#include "core/byte_array/byte_array.hpp"
#include "crypto/stream_hasher.hpp"
#include <openssl/sha.h>

namespace fetch {
namespace crypto {

class SHA256 : public StreamHasher
{
public:
  using StreamHasher::Update;
  using StreamHasher::Final;

  // Construction / Destruction
  SHA256();
  ~SHA256() override = default;

  static constexpr std::size_t size_in_bytes()
  {
    return SHA256_DIGEST_LENGTH;
  }

  /// @name Stream Hasher Interface
  /// @{
  void        Reset() override;
  bool        Update(uint8_t const *data_to_hash, std::size_t size) override;
  void        Final(uint8_t *hash, std::size_t size) override;
  std::size_t GetSizeInBytes() const override;
  /// @}

private:
  SHA256_CTX context_;
};

inline std::size_t SHA256::GetSizeInBytes() const
{
  return size_in_bytes();
}

}  // namespace crypto
}  // namespace fetch
