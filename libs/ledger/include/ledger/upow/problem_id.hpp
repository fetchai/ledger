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

#include "chain/address.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"

#include <cstddef>
#include <functional>

namespace fetch {
namespace ledger {

struct ProblemId
{
  chain::Address const             contract_address;
  byte_array::ConstByteArray const contract_digest;

  bool operator<(ProblemId const &other) const
  {
    if (contract_digest == other.contract_digest)
    {
      return contract_address.address() < other.contract_address.address();
    }

    return contract_digest < other.contract_digest;
  }

  bool operator==(ProblemId const &other) const
  {
    return contract_address == other.contract_address && contract_digest == other.contract_digest;
  }
};

}  // namespace ledger
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::ledger::ProblemId>
{
  std::size_t operator()(fetch::ledger::ProblemId const &id) const noexcept
  {
    std::hash<fetch::chain::Address>             a;
    std::hash<fetch::byte_array::ConstByteArray> b;

    return a(id.contract_address) + b(id.contract_digest);
  }
};

}  // namespace std
