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

#include "core/serializers/stl_types.hpp"
#include "core/service_ids.hpp"
#include "ledger/chain/main_chain.hpp"
#include "network/service/protocol.hpp"

namespace fetch {
namespace ledger {

class MainChainProtocol : public service::Protocol
{
public:
  using Blocks = std::vector<Block>;

  enum
  {
    HEAVIEST_CHAIN   = 1,
    COMMON_SUB_CHAIN = 2,
    TIME_TRAVEL      = 3
  };

  explicit MainChainProtocol(MainChain &chain)
    : chain_(chain)
  {
    Expose(HEAVIEST_CHAIN, this, &MainChainProtocol::GetHeaviestChain);
    Expose(COMMON_SUB_CHAIN, this, &MainChainProtocol::GetCommonSubChain);
    Expose(TIME_TRAVEL, this, &MainChainProtocol::TimeTravel);
  }

private:
  Blocks GetHeaviestChain(uint64_t maxsize) const
  {
    LOG_STACK_TRACE_POINT;
    return Copy(chain_.GetHeaviestChain(maxsize));
  }

  Blocks GetCommonSubChain(Digest start, Digest last_seen, uint64_t limit) const
  {
    LOG_STACK_TRACE_POINT;

    MainChain::Blocks blocks;

    if (!chain_.GetPathToCommonAncestor(blocks, std::move(start), std::move(last_seen), limit))
    {
      return Blocks{};
    }
    return Copy(blocks);
  }

  Blocks TimeTravel(Digest start, int64_t limit) const
  {
    LOG_STACK_TRACE_POINT;
    return Copy(chain_.TimeTravel(std::move(start), limit));
  }

  static Blocks Copy(MainChain::Blocks const &blocks)
  {
    Blocks output{};
    output.reserve(blocks.size());

    for (auto const &block : blocks)
    {
      output.emplace_back(*block);
    }

    return output;
  }

  MainChain &chain_;
};

}  // namespace ledger
}  // namespace fetch
