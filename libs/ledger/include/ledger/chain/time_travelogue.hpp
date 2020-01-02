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
#include "core/serializers/base_types.hpp"
#include "ledger/chain/block.hpp"

namespace fetch {
namespace ledger {

enum class TravelogueStatus : uint8_t
{
  HEAVIEST_BRANCH = 0,
  SIDE_BRANCH,
  NOT_FOUND,
};

/**
 * Packet format used to convey heaviest chain for node sync.
 */
template <class B>
struct TimeTravelogue
{
  using Block     = B;
  using BlockHash = Digest;
  using Blocks    = std::vector<Block>;

  /// @name Heaviest Block Information
  /// @{
  BlockHash heaviest_hash{};
  uint64_t  block_number{0};
  /// @}

  /// @name Request Metadata
  /// @{
  TravelogueStatus status{TravelogueStatus::NOT_FOUND};
  Blocks           blocks{};
  /// @}
};

}  // namespace ledger

namespace serializers {

template <class B, class D>
struct MapSerializer<ledger::TimeTravelogue<B>, D>
{
  using Type       = ledger::TimeTravelogue<B>;
  using DriverType = D;

  static constexpr uint8_t BLOCKS        = 1;
  static constexpr uint8_t HEAVIEST_HASH = 2;
  static constexpr uint8_t BLOCK_NUMBER  = 3;
  static constexpr uint8_t STATUS        = 4;

  template <class Constructor>
  static void Serialize(Constructor &map_constructor, Type const &travelogue)
  {
    auto map = map_constructor(4);

    auto const status_code = static_cast<uint8_t>(travelogue.status);

    map.Append(BLOCKS, travelogue.blocks);
    map.Append(HEAVIEST_HASH, travelogue.heaviest_hash);
    map.Append(BLOCK_NUMBER, travelogue.block_number);
    map.Append(STATUS, status_code);
  }

  template <class MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &travelogue)
  {
    uint8_t status_code{0};

    map.ExpectKeyGetValue(BLOCKS, travelogue.blocks);
    map.ExpectKeyGetValue(HEAVIEST_HASH, travelogue.heaviest_hash);
    map.ExpectKeyGetValue(BLOCK_NUMBER, travelogue.block_number);
    map.ExpectKeyGetValue(STATUS, status_code);

    travelogue.status = static_cast<ledger::TravelogueStatus>(status_code);
  }
};

}  // namespace serializers
}  // namespace fetch
