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

#include "ledger/chain/time_travelogue.hpp"
#include "muddle/address.hpp"
#include "network/generics/promise_of.hpp"

namespace fetch {
namespace ledger {

class MainChainRpcClientInterface
{
public:
  using Travelogue        = TimeTravelogue<Block>;
  using Blocks            = Travelogue::Blocks;
  using MuddleAddress     = muddle::Address;
  using BlocksPromise     = network::PromiseOf<Blocks>;
  using TraveloguePromise = network::PromiseOf<Travelogue>;

  MainChainRpcClientInterface()          = default;
  virtual ~MainChainRpcClientInterface() = default;

  /// @name Main Chain Rpc Protocol
  /// @{
  virtual BlocksPromise     GetHeaviestChain(MuddleAddress peer, uint64_t max_size) = 0;
  virtual BlocksPromise     GetCommonSubChain(MuddleAddress peer, Digest start, Digest last_seen,
                                              uint64_t limit)                       = 0;
  virtual TraveloguePromise TimeTravel(MuddleAddress peer, Digest start)            = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
