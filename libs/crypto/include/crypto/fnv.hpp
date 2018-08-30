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

class FNV : public StreamHasher
{
public:
  using context_type = std::size_t;

private:
  context_type                 context_;

public:

  using StreamHasher::Update;
  using StreamHasher::Final;

  FNV();

  void        Reset() override;
  bool        Update(uint8_t const *data_to_hash, std::size_t const &size) override;
  void        Final(uint8_t *hash, std::size_t const &size) override;
  std::size_t hashSize() const override;
};

struct CallableFNV
{
  std::size_t operator()(fetch::byte_array::ConstByteArray const &key) const;
};

}  // namespace crypto
}  // namespace fetch
