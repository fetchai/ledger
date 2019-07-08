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

  static_assert(FNV::size_in_bytes == ImplType::size_in_bytes,
                "Incorrect value of FNV::size_in_bytes");
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

bool FNV::ResetHasherInternal()
{
  impl_->ctx.reset();

  return true;
}

bool FNV::UpdateHasherInternal(uint8_t const *data_to_hash, std::size_t const size)
{
  impl_->ctx.update(data_to_hash, size);

  return true;
}

bool FNV::FinaliseHasherInternal(uint8_t *const hash)
{
  auto hash_ptr = reinterpret_cast<internal::FnvHasherInternals::ImplType::number_type *>(hash);
  *hash_ptr     = impl_->ctx.context();

  return true;
}

}  // namespace crypto
}  // namespace fetch
