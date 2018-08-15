#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
  using proof_type = fetch::chain::MainChain::proof_type;
  using block_type = fetch::chain::MainChain::block_type;
  using block_hash = fetch::chain::MainChain::block_hash;

  enum
  {
    protocol_number = fetch::protocols::FetchProtocols::MAIN_CHAIN
  };
  using protocol_class_type = fetch::protocols::MainChainProtocol;

  MainChainNodeInterface()          = default;
  virtual ~MainChainNodeInterface() = default;

  virtual std::pair<bool, block_type> GetHeader(const block_hash &hash)  = 0;
  virtual std::vector<block_type>     GetHeaviestChain(uint32_t maxsize) = 0;
};

}  // namespace ledger
}  // namespace fetch
