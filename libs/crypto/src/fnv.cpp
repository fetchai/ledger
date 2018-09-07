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

namespace {
constexpr std::size_t hash_size = sizeof(FNV::context_type);
static_assert(hash_size == 4 || hash_size == 8,
              "Hash size (= sizeof(std::size_t)) must be either 4 or 8 bytes.");

constexpr FNV::context_type fnv_prime =
    hash_size == 4 ? (1ull << 24) + (1ull << 8) + 0x93ull : (1ull << 40) + (1ull << 8) + 0xb3ull;
constexpr FNV::context_type fnv_offset_basis = hash_size == 4 ? 0x811c9dc5 : 0xcbf29ce484222325;

void update(FNV::context_type &context, uint8_t const *data_to_hash, std::size_t const &size)
{
  for (std::size_t i = 0; i < size; ++i)
  {
    context ^= data_to_hash[i];
    context *= fnv_prime;
  }
}

void reset(FNV::context_type &context)
{
  context = fnv_offset_basis;
}

}  // namespace

FNV::FNV()
{
  reset(context_);
}

std::size_t FNV::hashSize() const
{
  return hash_size;
}

void FNV::Reset()
{
  reset(context_);
}

bool FNV::Update(uint8_t const *data_to_hash, std::size_t const &size)
{
  update(context_, data_to_hash, size);
  return true;
}

void FNV::Final(uint8_t *hash, std::size_t const &size)
{
  if (size < hash_size)
  {
    throw std::runtime_error("size of input buffer is smaller than hash size.");
  }

  auto hash_ptr = reinterpret_cast<std::size_t *>(hash);
  *hash_ptr     = context_;
}

std::size_t CallableFNV::operator()(fetch::byte_array::ConstByteArray const &key) const
{
  FNV::context_type hash;
  reset(hash);
  update(hash, key.pointer(), key.size());
  return hash;
}

}  // namespace crypto
}  // namespace fetch
