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

#include "core/service_ids.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/time_travelogue.hpp"
#include "meta/value_util.hpp"
#include "network/service/protocol.hpp"

#include <algorithm>
#include <tuple>
#include <utility>

namespace fetch {
namespace ledger {

class MainChainProtocol : public service::Protocol
{
public:
  using Travelogue = TimeTravelogue<Block>;
  using Blocks     = Travelogue::Blocks;

  enum
  {
    HEAVIEST_CHAIN   = 1,
    TIME_TRAVEL      = 2,
    COMMON_SUB_CHAIN = 3
  };

  explicit MainChainProtocol(MainChain &chain)
    : chain_(chain)
  {
    Expose(HEAVIEST_CHAIN, this, &MainChainProtocol::GetHeaviestChain);
    Expose(COMMON_SUB_CHAIN, this, &MainChainProtocol::GetCommonSubChain);
    Expose(TIME_TRAVEL, this, &MainChainProtocol::TimeTravel);
  }

private:
  Blocks GetHeaviestChain(uint64_t last_known_block, uint64_t limit)
  {
    auto heaviest_block(chain_.GetHeaviestBlock());
    if (heaviest_block)
    {
      auto heaviest_number{heaviest_block->body.block_number};
      if (heaviest_number > last_known_block)
      {
        auto requested_amount = std::min(limit, heaviest_number - last_known_block);
        return InverseCopy(chain_.GetHeaviestChain(requested_amount));
      }
      if (heaviest_number == last_known_block)
      {
        return Blocks{*heaviest_block};
      }
    }
    return Blocks{};
  }

  Blocks GetCommonSubChain(Digest start, Digest last_seen, uint64_t limit)
  {
    MainChain::Blocks blocks;

    // TODO(issue 1725): this can cause issue if it doesn't exist (?)
    if (!chain_.GetPathToCommonAncestor(blocks, std::move(start), std::move(last_seen), limit))
    {
      return Blocks{};
    }
    return InverseCopy(blocks);
  }

  Travelogue TimeTravel(Digest start, int64_t limit)
  {
    auto rv = chain_.TimeTravel(std::move(start), limit);
    auto returned_blocks{limit > 0 ? Copy(rv.blocks) : InverseCopy(rv.blocks)};
    return {std::move(returned_blocks), std::move(rv.next_hash), rv.proceed_in_this_direction};
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
}  // namespace fetch
