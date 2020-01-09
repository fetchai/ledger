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

#include "chain/constants.hpp"
#include "ledger/chain/block.hpp"

namespace fetch {
namespace ledger {

/**
 * The class represents block structure in permanent chain storage file.
 */
struct BlockDbRecord
{
  Block block;
  // empty next hash is used as undefined value
  Block::Hash next_hash;

  Block::Hash hash() const
  {
    return block.hash;
  }
};

}  // namespace ledger

namespace serializers {

template <typename V, typename D>
struct MapSerializer;

template <typename D>
struct MapSerializer<ledger::BlockDbRecord, D>
{
public:
  using Type       = ledger::BlockDbRecord;
  using DriverType = D;

  static uint8_t const BLOCK     = 1;
  static uint8_t const NEXT_HASH = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &dbRecord)
  {
    auto map = map_constructor(2);
    map.Append(BLOCK, dbRecord.block);
    map.Append(NEXT_HASH, dbRecord.next_hash);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &dbRecord)
  {
    map.ExpectKeyGetValue(BLOCK, dbRecord.block);
    map.ExpectKeyGetValue(NEXT_HASH, dbRecord.next_hash);
  }
};

}  // namespace serializers
}  // namespace fetch
