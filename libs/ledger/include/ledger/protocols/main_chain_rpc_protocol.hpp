#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
  using Block     = chain::MainChain::BlockType;
  using BlockList = std::vector<Block>;

  enum
  {
    HEAVIEST_CHAIN = 1,
  };

  explicit MainChainProtocol(chain::MainChain &chain)
    : chain_(chain)
  {
    Expose(HEAVIEST_CHAIN, this, &MainChainProtocol::GetHeaviestChain);
  }

private:
  // TODO(EJF): Not really sure why I can't just call this function directly (default argument?)
  std::vector<Block> GetHeaviestChain(uint32_t const &maxsize)
  {
    LOG_STACK_TRACE_POINT;
    return chain_.HeaviestChain(maxsize);
  }

  chain::MainChain &chain_;
};

}  // namespace ledger
}  // namespace fetch
