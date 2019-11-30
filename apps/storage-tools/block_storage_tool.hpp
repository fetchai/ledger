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

#include "block_storage_client.hpp"
#include "core/reactor.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/protocols/main_chain_rpc_service.hpp"
#include "muddle/muddle_interface.hpp"
#include "network/management/network_manager.hpp"

#include <memory>

class BlockStorageTool
{
public:
  // Construction / Destruction
  BlockStorageTool();
  BlockStorageTool(BlockStorageTool const &) = delete;
  BlockStorageTool(BlockStorageTool &&)      = delete;
  ~BlockStorageTool();

  int Run();

  // Operators
  BlockStorageTool &operator=(BlockStorageTool const &) = delete;
  BlockStorageTool &operator=(BlockStorageTool &&) = delete;

private:
  using MuddlePtr              = fetch::muddle::MuddlePtr;
  using NetworkManager         = fetch::network::NetworkManager;
  using MainChain              = fetch::ledger::MainChain;
  using MainChainRpcService    = fetch::ledger::MainChainRpcService;
  using Reactor                = fetch::core::Reactor;
  using MainChainRpcServicePtr = std::shared_ptr<MainChainRpcService>;

  void Sync();
  void BroadcastLatest();

  NetworkManager         nm_{"main", 1};
  MuddlePtr              net_;
  MainChain              chain_{false, MainChain::Mode::LOAD_PERSISTENT_DB};
  Reactor                reactor_{"Reactor"};
  MainChainRpcServicePtr service_;
};
