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

#include "chain/constants.hpp"
#include "core/serializers/base_types.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"

namespace fetch {
namespace ledger {

/**
 * Packet format used to convey heaviest chain for node sync.
 */
struct TimeTravelogue
{
  using Block     = MainChain::BlockPtr;
  using BlockHash = Digest;
  using Blocks    = std::vector<Block>;

  Blocks    blocks;
  BlockHash heaviest_hash;
};

}  // namespace ledger

namespace serializers {

template <class D>
struct MapSerializer<ledger::TimeTravelogue, D>
{
  using Type       = ledger::TimeTravelogue;
  using DriverType = D;

  static constexpr uint8_t BLOCKS        = 1;
  static constexpr uint8_t HEAVIEST_HASH = 2;

  template <class Constructor>
  static void Serialize(Constructor &map_constructor, Type const &travelogue)
  {
    auto map = map_constructor(2);
    map.Append(BLOCKS, travelogue.blocks);
    map.Append(HEAVIEST_HASH, travelogue.heaviest_hash);
  }

  template <class MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &travelogue)
  {
    map.ExpectKeyGetValue(BLOCKS, travelogue.blocks);
    map.ExpectKeyGetValue(HEAVIEST_HASH, travelogue.heaviest_hash);
  }
};

}  // namespace serializers
}  // namespace fetch
