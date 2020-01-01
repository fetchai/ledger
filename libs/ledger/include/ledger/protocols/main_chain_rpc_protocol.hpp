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

#include "core/serializers/base_types.hpp"
#include "core/service_ids.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/time_travelogue.hpp"
#include "network/service/protocol.hpp"

namespace fetch {
namespace ledger {

class MainChainProtocol : public service::Protocol
{
public:
  using Travelogue                          = TimeTravelogue<Block>;
  using Blocks                              = Travelogue::Blocks;
  static constexpr char const *LOGGING_NAME = "MainChainProtocol";

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

  Blocks GetHeaviestChain(uint64_t maxsize)
  {
    return Copy(chain_.GetHeaviestChain(maxsize));
  }

  Blocks GetCommonSubChain(Digest start, Digest last_seen, uint64_t limit)
  {
    MainChain::Blocks blocks;

    // TODO(issue 1725): this can cause issue if it doesn't exist (?)
    if (!chain_.GetPathToCommonAncestor(blocks, std::move(start), std::move(last_seen), limit))
    {
      return Blocks{};
    }

    return Copy(blocks);
  }

  Travelogue TimeTravel(Digest start)
  {
    auto const ret_val = chain_.TimeTravel(std::move(start));

    // make a copy (because you need to convert from BlockPtr and Blocks)!
    return {ret_val.heaviest_hash, ret_val.block_number, ret_val.status, Copy(ret_val.blocks)};
  }

private:
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

  MainChain &chain_;
};

}  // namespace ledger
}  // namespace fetch
