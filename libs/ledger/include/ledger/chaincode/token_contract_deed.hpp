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

#include "core/byte_array/decoders.hpp"
#include "crypto/fnv.hpp"

namespace fetch {
namespace chain {
class VerifiedTransaction;
}

namespace ledger {

using Address = byte_array::ConstByteArray;
using Weight  = std::size_t;

struct Deed
{
  using DeedOperation      = byte_array::ConstByteArray;
  using Signees            = std::unordered_map<Address, Weight>;
  using OperationTresholds = std::unordered_map<DeedOperation, Weight>;

  bool IsSane() const;
  bool Verify(chain::VerifiedTransaction const &tx, DeedOperation const &operation) const;

  Deed()             = default;
  Deed(Deed const &) = default;
  Deed(Deed &&)      = default;

  Deed(Signees signees, OperationTresholds thresholds);

  Deed &operator=(Deed const &left) = default;
  Deed &operator=(Deed &&left) = default;

private:
  Signees            signees_;
  OperationTresholds operationTresholds_;
  Weight             full_weight_{0};

  template <typename T>
  friend void Serialize(T &serializer, Deed const &b)
  {
    serializer << b.signees_ << b.operationTresholds_;
  }

  template <typename T>
  friend void Deserialize(T &serializer, Deed &b)
  {
    serializer >> b.signees_ >> b.operationTresholds_;
  }
};

}  // namespace ledger
}  // namespace fetch