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

#include "core/logger.hpp"
#include "core/service_ids.hpp"
#include "ledger/shard_config.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_identity.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "network/generics/atomic_state_machine.hpp"
#include "network/generics/backgrounded_work.hpp"
#include "network/generics/future_timepoint.hpp"
#include "network/generics/has_worker_thread.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/service_client.hpp"

#include "ledger/chain/transaction.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/object_store_protocol.hpp"

#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"

#include "crypto/merkle_tree.hpp"

#include <chrono>
#include <thread>
#include <utility>

namespace fetch {
namespace ledger {

class StorageUnitClient final : public StorageUnitInterface
{
public:
  using MuddleEndpoint = muddle::MuddleEndpoint;
  using Address        = MuddleEndpoint::Address;

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
  void AddTransaction(VerifiedTransaction const &tx) override;
  void AddTransactions(TransactionList const &txs) override;

  TxSummaries PollRecentTx(uint32_t max_to_poll) override;

  bool GetTransaction(ConstByteArray const &digest, Transaction &tx) override;
  bool HasTransaction(ConstByteArray const &digest) override;

  Document GetOrCreate(ResourceAddress const &key) override;
  Document Get(ResourceAddress const &key) override;
  void     Set(ResourceAddress const &key, StateValue const &value) override;

  // state hash functions
  byte_array::ConstByteArray CurrentHash() override;
  byte_array::ConstByteArray LastCommitHash() override;
  bool                       RevertToHash(Hash const &hash) override;
  byte_array::ConstByteArray Commit() override;
  bool                       HashExists(Hash const &hash) override;
  bool                       Lock(ResourceAddress const &key) override;
  bool                       Unlock(ResourceAddress const &key) override;
  /// @}

  StorageUnitClient &operator=(StorageUnitClient const &) = delete;
  StorageUnitClient &operator=(StorageUnitClient &&) = delete;

private:
  using Client        = muddle::rpc::Client;
  using ClientPtr     = std::shared_ptr<Client>;
  using LaneIndex     = LaneIdentity::lane_type;
  using AddressList   = std::vector<MuddleEndpoint::Address>;
  using MerkleTree    = crypto::MerkleTree;
  using MerkleTreePtr = std::shared_ptr<MerkleTree>;
  using MerkleStack   = std::deque<MerkleTreePtr>;
  using Mutex         = fetch::mutex::Mutex;

  Address const &LookupAddress(LaneIndex lane) const;
  Address const &LookupAddress(storage::ResourceID const &resource) const;

  bool HashInStack(Hash const &hash);

  /// @name Client Information
  /// @{
  AddressList const addresses_;
  uint32_t const    log2_num_lanes_ = 0;
  Client            rpc_client_;
  /// @}

  /// @name State Hash Support
  /// @{
  mutable Mutex merkle_mutex_{__LINE__, __FILE__};
  MerkleTreePtr current_merkle_{};
  MerkleStack   state_merkle_stack_{};
  /// @}
};

inline uint32_t StorageUnitClient::num_lanes() const
{
  return 1u << log2_num_lanes_;
}

}  // namespace ledger
}  // namespace fetch
