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
#include "ledger/chain/address.hpp"
#include "ledger/chain/address_rpc_serializer.hpp"

namespace fetch {
namespace ledger {

class Transaction;

struct Deed
{
  using Weight             = std::size_t;
  using Threshold          = Weight;
  using DeedOperation      = byte_array::ConstByteArray;
  using Signees            = std::unordered_map<Address, Weight>;
  using OperationTresholds = std::unordered_map<DeedOperation, Threshold>;
  using Weights            = std::unordered_map<Weight, std::size_t>;
  using MandatorityMatrix  = std::unordered_map<Threshold, Weights>;

  bool              IsSane() const;
  bool              Verify(Transaction const &tx, DeedOperation const &operation) const;
  MandatorityMatrix InferMandatoryWeights() const;

  Deed()             = default;
  Deed(Deed const &) = default;
  Deed(Deed &&)      = default;

  Deed(Signees signees, OperationTresholds thresholds);

  Deed &operator=(Deed const &left) = default;
  Deed &operator=(Deed &&left) = default;

private:
  Signees            signees_;
  OperationTresholds operation_thresholds_;
  // Derived data:
  Weight full_weight_{0};

  template <typename T>
  friend void Serialize(T &serializer, Deed const &b)
  {
    serializer << b.signees_ << b.operation_thresholds_;
  }

  template <typename T>
  friend void Deserialize(T &serializer, Deed &b)
  {
    serializer >> b.signees_ >> b.operation_thresholds_;
  }
};

}  // namespace ledger
}  // namespace fetch
