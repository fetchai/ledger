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

#include "core/future_timepoint.hpp"
#include "core/service_ids.hpp"
#include "crypto/merkle_tree.hpp"
#include "ledger/shard_config.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "ledger/storage_unit/object_store_protocol.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "logging/logging.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "network/generics/backgrounded_work.hpp"
#include "network/generics/has_worker_thread.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/object_stack.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace ledger {

class StorageUnitClient final : public StorageUnitInterface
{
public:
  using MuddleEndpoint = muddle::MuddleEndpoint;
  using Address        = muddle::Address;

  static constexpr char const *LOGGING_NAME = "StorageUnitClient";

  // Construction / Destruction
  StorageUnitClient(MuddleEndpoint &muddle, ShardConfigs const &shards, uint32_t log2_num_lanes);
  StorageUnitClient(StorageUnitClient const &) = delete;
  StorageUnitClient(StorageUnitClient &&)      = delete;
  ~StorageUnitClient() override                = default;

  // Helpers
  uint32_t num_lanes() const;

  /// @name Storage Unit Interface
  /// @{
  void      AddTransaction(chain::Transaction const &tx) override;
  bool      GetTransaction(ConstByteArray const &digest, chain::Transaction &tx) override;
  bool      HasTransaction(ConstByteArray const &digest) override;
  void      IssueCallForMissingTxs(DigestSet const &tx_set) override;
  TxLayouts PollRecentTx(uint32_t max_to_poll) override;

  Document GetOrCreate(ResourceAddress const &key) override;
  Document Get(ResourceAddress const &key) const override;
  void     Set(ResourceAddress const &key, StateValue const &value) override;

  void Reset() override;

  // state hash functions
  byte_array::ConstByteArray CurrentHash() override;
  byte_array::ConstByteArray LastCommitHash() override;
  bool                       RevertToHash(Hash const &hash, uint64_t index) override;
  byte_array::ConstByteArray Commit(uint64_t commit_index) override;
  bool                       HashExists(Hash const &hash, uint64_t index) override;
  bool                       Lock(ShardIndex index) override;
  bool                       Unlock(ShardIndex index) override;
  /// @}

  StorageUnitClient &operator=(StorageUnitClient const &) = delete;
  StorageUnitClient &operator=(StorageUnitClient &&) = delete;

private:
  using Client               = muddle::rpc::Client;
  using ClientPtr            = std::shared_ptr<Client>;
  using LaneIndex            = uint32_t;
  using AddressList          = std::vector<muddle::Address>;
  using MerkleTree           = crypto::MerkleTree;
  using PermanentMerkleStack = fetch::storage::ObjectStack<crypto::MerkleTree>;

  Address const &LookupAddress(ShardIndex shard) const;
  Address const &LookupAddress(storage::ResourceID const &resource) const;

  bool HashInStack(Hash const &hash, uint64_t index);

  /// @name Client Information
  /// @{
  AddressList const addresses_;
  uint32_t const    log2_num_lanes_ = 0;
  ClientPtr         rpc_client_;
  /// @}

  /// @name State Hash Support
  /// @{
  mutable Mutex        merkle_mutex_;
  MerkleTree           current_merkle_;
  PermanentMerkleStack permanent_state_merkle_stack_{};
  /// @}
};

}  // namespace ledger
}  // namespace fetch
