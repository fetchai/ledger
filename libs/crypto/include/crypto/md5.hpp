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
#include "crypto/openssl_hasher.hpp"

namespace fetch {
namespace crypto {

class MD5 : public HasherInterface
{
public:
  using HasherInterface::Update;
  using HasherInterface::Final;

  static constexpr std::size_t size_in_bytes = 16u;

  MD5()            = default;
  ~MD5() override  = default;
  MD5(MD5 const &) = delete;
  MD5(MD5 &&)      = delete;

  MD5 &operator=(MD5 const &) = delete;
  MD5 &operator=(MD5 &&) = delete;

  void        Reset() override;
  bool        Update(uint8_t const *data_to_hash, std::size_t size) override;
  void        Final(uint8_t *hash) override;
  std::size_t HashSizeInBytes() const override;

private:
  internal::OpenSslHasher openssl_hasher_{internal::OpenSslDigestType::MD5};
};

}  // namespace crypto
}  // namespace fetch
