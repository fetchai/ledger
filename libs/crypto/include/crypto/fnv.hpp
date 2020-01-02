#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv_detail.hpp"
#include "crypto/hasher_interface.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace fetch {
namespace crypto {

namespace internal {
class FnvHasherInternals;
}

class FNV : public HasherInterface
{
public:
  using HasherInterface::Final;
  using HasherInterface::Update;

  static constexpr std::size_t SIZE_IN_BYTES = 8u;

  FNV();
  ~FNV() override  = default;
  FNV(FNV const &) = delete;
  FNV(FNV &&)      = delete;

  FNV &operator=(FNV const &) = delete;
  FNV &operator=(FNV &&) = delete;

  void        Reset() override;
  bool        Update(uint8_t const *data_to_hash, std::size_t size) override;
  void        Final(uint8_t *hash) override;
  std::size_t HashSizeInBytes() const override;

private:
  std::shared_ptr<internal::FnvHasherInternals> impl_;
};

}  // namespace crypto
}  // namespace fetch
