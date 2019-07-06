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
#include "crypto/fnv_detail.hpp"
#include "crypto/stream_hasher.hpp"

namespace fetch {
namespace crypto {

class FNV : public StreamHasher<FNV>
{
public:
  using base_type      = StreamHasher<FNV>;
  using base_impl_type = detail::FNV1a;
  using context_type   = typename base_impl_type::number_type;

  constexpr static std::size_t size_in_bytes = base_impl_type::size_in_bytes;

private:
  void ResetHasher();
  bool UpdateHasher(uint8_t const *data_to_hash, std::size_t const &size);
  void FinalHasher(uint8_t *hash, std::size_t const &size);

  detail::FNV1a ctx_;

  friend base_type;
};

}  // namespace crypto
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::byte_array::ConstByteArray>
{
  std::size_t operator()(fetch::byte_array::ConstByteArray const &value) const
  {
    fetch::crypto::detail::FNV1a hash;
    hash.update(value.pointer(), value.size());
    return hash.context();
  }
};

template <>
struct hash<fetch::byte_array::ByteArray> : public hash<fetch::byte_array::ConstByteArray>
{
};

}  // namespace std
