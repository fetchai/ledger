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

namespace fetch {
namespace crypto {

class FNVLL : public virtual StreamHasherLowLevel
{
  uint32_t context_;

protected:
  using context_type = uint32_t;
  static const std::size_t hash_size;

public:
  std::size_t hashSize() const override;
  void        Reset() override;
  bool        Update(uint8_t const *data_to_hash, std::size_t const &size) override;
  void        Final(uint8_t *hash, std::size_t const &size) override;
  uint32_t    uint_digest() const;
};

class FNV : public FNVLL, public virtual StreamHasher
{
  byte_array_type digest_;

public:
  using byte_array_type = typename StreamHasher::byte_array_type;

  using FNVLL::Update;
  using FNVLL::Final;

  FNV();
  bool            Update(byte_array_type const &s) override;
  void            Final() override;
  byte_array_type digest() const override;
};

struct CallableFNV
{
  std::size_t operator()(fetch::byte_array::ConstByteArray const &key) const;
};

}  // namespace crypto
}  // namespace fetch
