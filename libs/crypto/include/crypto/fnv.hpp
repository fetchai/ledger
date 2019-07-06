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

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {

namespace internal {
class FnvHasherInternals;
}

class FNV : private StreamHasher<FNV>
{
public:
  using BaseType = StreamHasher<FNV>;

  using BaseType::Final;
  using BaseType::Reset;
  using BaseType::Update;

  static constexpr std::size_t size_in_bytes = 8u;

  FNV();
  ~FNV();

private:
  void ResetHasher();
  bool UpdateHasher(uint8_t const *data_to_hash, std::size_t const &size);
  void FinalHasher(uint8_t *hash, std::size_t const &size);

  internal::FnvHasherInternals *impl_;

  friend BaseType;
};

}  // namespace crypto
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::byte_array::ConstByteArray>
{
  std::size_t operator()(fetch::byte_array::ConstByteArray const &value) const
  {
    fetch::crypto::FNV hash;
    hash.Reset();
    hash.Update(value.pointer(), value.size());
    auto const ret = hash.Final();

    return *reinterpret_cast<std::size_t const *>(ret.pointer());
  }
};

template <>
struct hash<fetch::byte_array::ByteArray> : public hash<fetch::byte_array::ConstByteArray>
{
};

}  // namespace std
