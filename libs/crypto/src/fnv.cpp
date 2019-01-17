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

#include "crypto/fnv.hpp"

namespace fetch {
namespace crypto {

std::size_t FNV::GetSizeInBytes() const
{
  return base_impl_type::size_in_bytes;
}

void FNV::Reset()
{
  reset();
}

bool FNV::Update(uint8_t const *data_to_hash, std::size_t const &size)
{
  update(data_to_hash, size);
  return true;
}

void FNV::Final(uint8_t *hash, std::size_t const &size)
{
  if (size < base_impl_type::size_in_bytes)
  {
    throw std::runtime_error("size of input buffer is smaller than hash size.");
  }

  auto hash_ptr = reinterpret_cast<context_type *>(hash);
  *hash_ptr     = context();
}

}  // namespace crypto
}  // namespace fetch
