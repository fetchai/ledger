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
#include "crypto/fnv_detail.hpp"

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {
namespace internal {

class FnvHasherInternals
{
public:
  using ImplType = detail::FNV1a;
  ImplType ctx;
};

}  // namespace internal

FNV::FNV()
  : impl_(new internal::FnvHasherInternals)
{
  impl_->ctx.reset();
}

FNV::~FNV()
{
  delete impl_;
}

void FNV::Reset()
{
  impl_->ctx.reset();
}

bool FNV::Update(uint8_t const *const data_to_hash, std::size_t size)
{
  impl_->ctx.update(data_to_hash, size);

  return true;
}

void FNV::Final(uint8_t *const hash)
{
  auto hash_ptr = reinterpret_cast<internal::FnvHasherInternals::ImplType::number_type *>(hash);
  *hash_ptr     = impl_->ctx.context();
}

std::size_t FNV::HashSizeInBytes() const
{
  auto const size = internal::FnvHasherInternals::ImplType::size_in_bytes;
  static_assert(size == size_in_bytes, "Size mismatch");
  return size;
}

}  // namespace crypto
}  // namespace fetch
