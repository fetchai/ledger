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

#include "ledger/chain/block.hpp"
#include "muddle/address.hpp"
#include "muddle/rpc/client.hpp"

#include <vector>

class BlockStorageClient
{
public:
  using MuddleAddress  = fetch::muddle::Address;
  using MuddleEndpoint = fetch::muddle::MuddleEndpoint;
  using Blocks         = std::vector<fetch::ledger::Block>;

  // Construction / Destruction
  BlockStorageClient(MuddleAddress peer, MuddleEndpoint &endpoint);
  BlockStorageClient(BlockStorageClient const &) = delete;
  BlockStorageClient(BlockStorageClient &&)      = delete;
  ~BlockStorageClient()                          = default;

  // Complete chain download
  bool DownloadCompleteChain(Blocks &blocks);

  // Operators
  BlockStorageClient &operator=(BlockStorageClient const &) = delete;
  BlockStorageClient &operator=(BlockStorageClient &&) = delete;

private:
  using RpcClient = fetch::muddle::rpc::Client;

  MuddleAddress const peer_;
  RpcClient           client_;
};
