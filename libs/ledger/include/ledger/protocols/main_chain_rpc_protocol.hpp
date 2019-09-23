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

#include "core/serializers/base_types.hpp"
#include "core/service_ids.hpp"
#include "ledger/chain/main_chain.hpp"
#include "network/service/protocol.hpp"

#include <utility>

namespace fetch {
namespace ledger {

struct TimeTravelogue
{
  using Blocks = std::vector<Block>;

  Blocks blocks;
  Digest next_hash;
};

class MainChainProtocol : public service::Protocol
{
public:
  using Blocks = TimeTravelogue::Blocks;

  enum
  {
    HEAVIEST_CHAIN   = 1,
    TIME_TRAVEL      = 2,
    COMMON_SUB_CHAIN = 3,
  };

  explicit MainChainProtocol(MainChain &chain)
    : chain_(chain)
  {
    Expose(HEAVIEST_CHAIN, this, &MainChainProtocol::GetHeaviestChain);
    Expose(COMMON_SUB_CHAIN, this, &MainChainProtocol::GetCommonSubChain);
    Expose(TIME_TRAVEL, this, &MainChainProtocol::TimeTravel);
  }

private:
  Blocks GetHeaviestChain(uint64_t maxsize)
  {
    return InverseCopy(chain_.GetHeaviestChain(maxsize));
  }

  Blocks GetCommonSubChain(Digest start, Digest last_seen, uint64_t limit)
  {
    MainChain::Blocks blocks;

    if (!chain_.GetPathToCommonAncestor(blocks, std::move(start), std::move(last_seen), limit))
    {
      return Blocks{};
    }
    return InverseCopy(blocks);
  }

  TimeTravelogue TimeTravel(Digest start, int64_t limit)
  {
    auto ret_pair = chain_.TimeTravel(std::move(start), limit);
    return TimeTravelogue{Copy(ret_pair.first), std::move(ret_pair.second)};
  }

  static Blocks Copy(MainChain::Blocks const &blocks)
  {
    Blocks output{};
    output.reserve(blocks.size());

    for (auto const &block : blocks)
    {
      output.push_back(*block);
    }

    return output;
  }

  static Blocks InverseCopy(MainChain::Blocks const &blocks)
  {
    Blocks output{};
    output.reserve(blocks.size());

    for (auto block_it = blocks.rbegin(); block_it != blocks.rend(); ++block_it)
    {
      output.push_back(**block_it);
    }

    return output;
  }

  MainChain &chain_;
};

}  // namespace ledger

namespace serializers {

template <class D>
struct MapSerializer<ledger::TimeTravelogue, D>
{
  using Type       = ledger::TimeTravelogue;
  using DriverType = D;

  static constexpr uint8_t BLOCKS    = 1;
  static constexpr uint8_t NEXT_HASH = 2;

  template <class Constructor>
  static void Serialize(Constructor &map_constructor, Type const &travel_log)
  {
    auto map = map_constructor(2);
    map.Append(BLOCKS, travel_log.blocks);
    map.Append(NEXT_HASH, travel_log.next_hash);
  }
  template <class Deserializer>
  static void Deserialize(Deserializer &map, Type &travel_log)
  {
    map.ExpectKeyGetValue(BLOCKS, travel_log.blocks);
    map.ExpectKeyGetValue(NEXT_HASH, travel_log.next_hash);
  }
};

}  // namespace serializers
}  // namespace fetch
