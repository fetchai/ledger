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

#include "muddle/muddle_interface.hpp"
#include "network/management/network_manager.hpp"
#include "tx_storage_client.hpp"

#include <cstdint>

class TxStorageTool
{
public:
  using FilenameSet    = std::unordered_set<std::string>;
  using NetworkManager = fetch::network::NetworkManager;
  using MuddlePtr      = fetch::muddle::MuddlePtr;

  // Construction / Destruction
  explicit TxStorageTool(uint32_t log2_num_lanes);
  TxStorageTool(TxStorageTool const &) = delete;
  TxStorageTool(TxStorageTool &&)      = delete;
  ~TxStorageTool();

  int Run(fetch::DigestSet const &tx_to_get, FilenameSet const &txs_to_set);

  // Operators
  TxStorageTool &operator=(TxStorageTool const &) = delete;
  TxStorageTool &operator=(TxStorageTool &&) = delete;

private:
  using ClientPtr = std::unique_ptr<TxStorageClient>;

  bool Download(fetch::Digest const &digest);
  bool Upload(std::string const &filename);

  uint32_t const log2_num_lanes_{0};
  uint32_t const num_lanes_{1u << log2_num_lanes_};
  NetworkManager nm_{"main", 1};
  MuddlePtr      net_;
  ClientPtr      client_;
};
