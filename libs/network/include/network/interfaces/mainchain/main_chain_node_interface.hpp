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

#include "ledger/chain/main_chain.hpp"
#include "network/protocols/fetch_protocols.hpp"

namespace fetch {
namespace protocols {
class MainChainProtocol;
}

namespace ledger {

class MainChainNodeInterface
{
public:
  using BlockType = fetch::chain::MainChain::BlockType;
  using BlockHash = fetch::chain::MainChain::BlockHash;

  enum
  {
    protocol_number = fetch::protocols::FetchProtocols::MAIN_CHAIN
  };
  using protocol_class_type = fetch::protocols::MainChainProtocol;

  MainChainNodeInterface()          = default;
  virtual ~MainChainNodeInterface() = default;

  virtual std::pair<bool, BlockType> GetHeader(const BlockHash &hash)   = 0;
  virtual std::vector<BlockType>     GetHeaviestChain(uint32_t maxsize) = 0;
};

}  // namespace ledger
}  // namespace fetch
