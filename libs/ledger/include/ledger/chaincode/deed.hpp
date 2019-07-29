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

namespace fetch {
namespace ledger {

class Transaction;

struct Deed
{
  using Weight             = uint64_t;
  using Threshold          = Weight;
  using DeedOperation      = byte_array::ConstByteArray;
  using Signees            = std::unordered_map<Address, Weight>;
  using OperationTresholds = std::unordered_map<DeedOperation, Threshold>;
  using Weights            = std::unordered_map<Weight, uint64_t>;
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

  template <typename T, typename D>
  friend struct serializers::MapSerializer;
};

}  // namespace ledger

namespace serializers {
template <typename D>
struct MapSerializer<ledger::Deed, D>
{
public:
  using Type       = ledger::Deed;
  using DriverType = D;

  static uint8_t const SIGNEES             = 1;
  static uint8_t const OPERATION_THRESHOLD = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &deed)
  {
    auto map = map_constructor(2);
    map.Append(SIGNEES, deed.signees_);
    map.Append(OPERATION_THRESHOLD, deed.operation_thresholds_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &deed)
  {
    map.ExpectKeyGetValue(SIGNEES, deed.signees_);
    map.ExpectKeyGetValue(OPERATION_THRESHOLD, deed.operation_thresholds_);
  }
};
}  // namespace serializers

}  // namespace fetch
