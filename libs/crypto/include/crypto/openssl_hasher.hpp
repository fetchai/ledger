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

#include "crypto/hasher_interface.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace fetch {

namespace byte_array {
class ConstByteArray;
}

namespace crypto {
namespace internal {

enum class OpenSslDigestType
{
  MD5,
  SHA1,
  SHA2_256,
  SHA2_512
};

class OpenSslHasherImpl;

class OpenSslHasher
{
public:
  explicit OpenSslHasher(OpenSslDigestType);
  ~OpenSslHasher()                     = default;
  OpenSslHasher(OpenSslHasher const &) = delete;
  OpenSslHasher(OpenSslHasher &&)      = delete;

  OpenSslHasher &operator=(OpenSslHasher const &) = delete;
  OpenSslHasher &operator=(OpenSslHasher &&) = delete;

  bool        Reset();
  bool        Update(uint8_t const *data_to_hash, std::size_t size);
  bool        Final(uint8_t *data_to_hash);
  std::size_t HashSize() const;

private:
  std::shared_ptr<internal::OpenSslHasherImpl> impl_;
};

}  // namespace internal
}  // namespace crypto
}  // namespace fetch
