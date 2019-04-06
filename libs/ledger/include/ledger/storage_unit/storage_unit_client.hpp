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

#include "core/serializers/typed_byte_array_buffer.hpp"
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
  bool                       RevertToHash(Hash const &hash, uint64_t index) override;
  byte_array::ConstByteArray Commit(uint64_t index) override;
  bool                       HashExists(Hash const &hash, uint64_t index) override;
  bool                       Lock(ResourceAddress const &key) override;
  bool                       Unlock(ResourceAddress const &key) override;
  /// @}

  StorageUnitClient &operator=(StorageUnitClient const &) = delete;
  StorageUnitClient &operator=(StorageUnitClient &&) = delete;

private:
  /**
   * On Disk representation of a merkle tree
   */
  struct MerkleTreeBlock
  {
    MerkleTreeBlock()
    {
      std::memset(this, -1, sizeof(*this));
    }

    explicit MerkleTreeBlock(crypto::MerkleTree const &tree)
    {
      serializers::TypedByteArrayBuffer buff;
      buff << tree;

      assert(buff.data().size() == 92 || buff.data().size() == 60);

      size_ = buff.data().size();
      std::memcpy((uint8_t *)data_, (uint8_t *)buff.data().pointer(), size_);
    }

    crypto::MerkleTree Extract(uint64_t num_lanes)
    {
      crypto::MerkleTree ret{num_lanes};

      serializers::TypedByteArrayBuffer buff;

      buff.Allocate(size_);

      std::memcpy((uint8_t *)buff.data().pointer(), (uint8_t *)data_, size_);

      try
      {
        buff >> ret;
      }
      catch (std::exception e)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "FAILED TO DESERIALIZE MERKLE TREE PROXY! ", size_);
        return ret;
      }

      return ret;
    }

    uint64_t size_{0};
    uint8_t  data_[92];
  };

  using Client               = muddle::rpc::Client;
  using ClientPtr            = std::shared_ptr<Client>;
  using LaneIndex            = LaneIdentity::lane_type;
  using AddressList          = std::vector<MuddleEndpoint::Address>;
  using MerkleTree           = crypto::MerkleTree;
  using PermanentMerkleStack = storage::RandomAccessStack<MerkleTreeBlock>;
  using Mutex                = fetch::mutex::Mutex;

  static constexpr char const *MERKLE_FILENAME = "merkle_stack.db";

  Address const &LookupAddress(LaneIndex lane) const;
  Address const &LookupAddress(storage::ResourceID const &resource) const;

  bool HashInStack(Hash const &hash, uint64_t index);

  /// @name Client Information
  /// @{
  AddressList const addresses_;
  uint32_t const    log2_num_lanes_ = 0;
  Client            rpc_client_;
  /// @}

  /// @name State Hash Support
  /// @{
  mutable Mutex        merkle_mutex_{__LINE__, __FILE__};
  MerkleTree           current_merkle_;
  PermanentMerkleStack permanent_state_merkle_stack_{};
  /// @}
};

inline uint32_t StorageUnitClient::num_lanes() const
{
  return 1u << log2_num_lanes_;
}

}  // namespace ledger
}  // namespace fetch
