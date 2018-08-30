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

#include "crypto/fnv.hpp"

namespace fetch {
namespace crypto {

const std::size_t FNV::hash_size_ { sizeof(context_type) };

FNV::FNV() { reset(); }

std::size_t FNV::hashSize() const { return hash_size_; }

inline void FNV::reset() { context_ = 2166136261; }

void FNV::Reset() { reset(); }

bool FNV::Update(uint8_t const *data_to_hash, std::size_t const &size)
{
  for (std::size_t i = 0; i < size; ++i)
  {
    context_ = (context_ * 16777619) ^ data_to_hash[i];
  }
  return true;
}

void FNV::Final(uint8_t *hash, std::size_t const &size)
{
  if (size < hash_size_) throw std::runtime_error("size of input buffer is smaller than hash size.");
  hash[0] = uint8_t(context_);
  hash[1] = uint8_t(context_ >> 8);
  hash[2] = uint8_t(context_ >> 16);
  hash[3] = uint8_t(context_ >> 24);
}

std::size_t CallableFNV::operator()(fetch::byte_array::ConstByteArray const &key) const
{
  uint32_t hash = 2166136261;
  for (std::size_t i = 0; i < key.size(); ++i)
  {
    hash = (hash * 16777619) ^ key[i];
  }

  return hash;
}

}  // namespace crypto
}  // namespace fetch
