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

#include "core/byte_array/const_byte_array.hpp"

#include <unordered_set>
#include <unordered_map>

namespace fetch {
namespace ledger {

using Digest = byte_array::ConstByteArray;

struct DigestHashAdapter
{
  std::size_t operator()(Digest const &hash) const
  {
    std::size_t value{0};

    if (!hash.empty())
    {
      assert(hash.size() >= sizeof(std::size_t));
      value = *reinterpret_cast<std::size_t const *>(hash.pointer());
    }

    return value;
  }
};

using DigestSet = std::unordered_set<Digest, DigestHashAdapter>;

template <typename Value>
using DigestMap = std::unordered_map<Digest, Value, DigestHashAdapter>;

}  // namespace ledger
}  // namespace fetch
