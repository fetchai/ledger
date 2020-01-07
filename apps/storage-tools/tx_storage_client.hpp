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

#include "chain/transaction.hpp"
#include "muddle/address.hpp"
#include "muddle/rpc/client.hpp"

#include <memory>

namespace fetch {
namespace storage {

class ResourceID;

}  // namespace storage
}  // namespace fetch

class TxStorageClient
{
public:
  using Transaction    = fetch::chain::Transaction;
  using MuddleAddress  = fetch::muddle::Address;
  using MuddleEndpoint = fetch::muddle::MuddleEndpoint;
  using LaneAddresses  = std::vector<MuddleAddress>;
  using Digest         = fetch::Digest;
  using DigestSet      = fetch::DigestSet;

  // Construction / Destruction
  TxStorageClient(LaneAddresses lane_addresses, MuddleEndpoint &endpoint);
  TxStorageClient(TxStorageClient const &) = delete;
  TxStorageClient(TxStorageClient &&)      = delete;
  ~TxStorageClient()                       = default;

  bool AddTransaction(Transaction const &tx);
  bool GetTransaction(Digest const &digest, Transaction &tx);
  bool HasTransaction(Digest const &digest);

  // Operators
  TxStorageClient &operator=(TxStorageClient const &) = delete;
  TxStorageClient &operator=(TxStorageClient &&) = delete;

private:
  using RpcClient = fetch::muddle::rpc::Client;

  MuddleAddress const &LookupAddress(fetch::storage::ResourceID const &resource) const;

  LaneAddresses const lane_addresses_;
  uint32_t            log2_num_lanes_;
  RpcClient           rpc_client_;
};
