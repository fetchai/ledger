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

#include "ledger/protocols/main_chain_rpc_client_interface.hpp"
#include "muddle/rpc/client.hpp"

namespace fetch {
namespace ledger {

class MainChainRpcClient : public MainChainRpcClientInterface
{
public:
  using MuddleEndpoint = muddle::MuddleEndpoint;

  // Construction / Destruction
  explicit MainChainRpcClient(MuddleEndpoint &endpoint);
  MainChainRpcClient(MainChainRpcClient const &) = delete;
  MainChainRpcClient(MainChainRpcClient &&)      = delete;
  ~MainChainRpcClient() override                 = default;

  /// @name Main Chain Rpc Protocol
  /// @{
  BlocksPromise     GetHeaviestChain(MuddleAddress peer, uint64_t max_size) override;
  BlocksPromise     GetCommonSubChain(MuddleAddress peer, Digest start, Digest last_seen,
                                      uint64_t limit) override;
  TraveloguePromise TimeTravel(MuddleAddress peer, Digest start) override;
  /// @}

  // Operators
  MainChainRpcClient &operator=(MainChainRpcClient const &) = delete;
  MainChainRpcClient &operator=(MainChainRpcClient &&) = delete;

private:
  using RpcClient = muddle::rpc::Client;

  RpcClient rpc_client_;
};

}  // namespace ledger
}  // namespace fetch
