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
#include "core/serializers/main_serializer.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "ledger/dag/dag_hash.hpp"

#include <set>

namespace fetch {
namespace ledger {

struct DAGEpoch
{
  using ConstByteArray = byte_array::ConstByteArray;

  DAGEpoch()
    : hash{{}, DAGHash::Type::EPOCH}
  {}

  uint64_t block_number{0};

  // TODO(issue 1229): The order of these nodes will need to be revised
  std::set<DAGHash> tips{};
  std::set<DAGHash> data_nodes{};
  std::set<DAGHash> solution_nodes{};

  DAGHash hash{};

  // TODO(HUT): consider whether to actually transmit
  // Not transmitted, but built up and compared against the hash for validity
  // map of dag node hash to dag node
  std::set<DAGHash> all_nodes;

  bool Contains(DAGHash const &digest) const;
  void Finalise();
};

}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::DAGEpoch, D>
{
public:
  using Type       = ledger::DAGEpoch;
  using DriverType = D;

  static uint8_t const BLOCK_NUMBER   = 0;
  static uint8_t const TIPS           = 1;
  static uint8_t const DATA_NODES     = 2;
  static uint8_t const SOLUTION_NODES = 3;
  static uint8_t const HASH           = 4;
  static uint8_t const ALL_NODES      = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &node)
  {
    auto map = map_constructor(6);

    map.Append(BLOCK_NUMBER, node.block_number);
    map.Append(TIPS, node.tips);
    map.Append(DATA_NODES, node.data_nodes);
    map.Append(SOLUTION_NODES, node.solution_nodes);
    map.Append(HASH, node.hash);
    map.Append(ALL_NODES, node.all_nodes);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &node)
  {
    map.ExpectKeyGetValue(BLOCK_NUMBER, node.block_number);
    map.ExpectKeyGetValue(TIPS, node.tips);
    map.ExpectKeyGetValue(DATA_NODES, node.data_nodes);
    map.ExpectKeyGetValue(SOLUTION_NODES, node.solution_nodes);
    map.ExpectKeyGetValue(HASH, node.hash);
    map.ExpectKeyGetValue(ALL_NODES, node.all_nodes);
  }
};

}  // namespace serializers
}  // namespace fetch
