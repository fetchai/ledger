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

#include "core/service_ids.hpp"
#include "core/logger.hpp"
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

class MuddleLaneConnectorWorker;

class StorageUnitClient final : public StorageUnitInterface
{
public:
  enum class State
  {
    INITIAL = 0,
    CONNECTING,
    QUERYING,
    SNOOZING,
    SUCCESS,
    TIMEDOUT,
    FAILED,
  };

  struct ClientDetails
  {
    crypto::Identity      identity;
    std::atomic<uint32_t> lane{0};
  };

  using Client    = muddle::rpc::Client;
  using CertificatePtr = std::shared_ptr<crypto::Prover>;
  using ClientPtr = std::shared_ptr<Client>;
  using MuddleEp  = muddle::MuddleEndpoint;
  using Muddle    = muddle::Muddle;
  using MuddlePtr = std::shared_ptr<Muddle>;
  using Address   = Muddle::Address;
  using Uri       = Muddle::Uri;
  using Peer      = fetch::network::Peer;

  using MuddleEndpoint = muddle::MuddleEndpoint;

  using LaneIndex            = LaneIdentity::lane_type;
  using ServiceClient        = service::ServiceClient;
  using SharedServiceClient  = std::shared_ptr<ServiceClient>;
  using WeakServiceClient    = std::weak_ptr<ServiceClient>;
  using SharedServiceClients = std::map<LaneIndex, SharedServiceClient>;
  using LaneToIdentity       = std::map<LaneIndex, Address>;
  using ClientRegister       = fetch::network::ConnectionRegister<ClientDetails>;
  using Handle               = ClientRegister::connection_handle_type;
  using NetworkManager       = fetch::network::NetworkManager;
  using PromiseState         = fetch::service::PromiseState;
  using Promise              = service::Promise;
  using FutureTimepoint      = network::FutureTimepoint;
  using Mutex                = fetch::mutex::Mutex;
  using LockT                = std::lock_guard<Mutex>;

  static constexpr char const *LOGGING_NAME = "StorageUnitClient";

  // Construction / Destruction
  StorageUnitClient(MuddleEndpoint &muddle, ShardConfigs const &shards, uint32_t log2_num_lanes);
  StorageUnitClient(StorageUnitClient const &) = delete;
  StorageUnitClient(StorageUnitClient &&)      = delete;
  ~StorageUnitClient() override = default;

//  void SetNumberOfLanes(LaneIndex count);

  void Start(uint32_t timeout_ms);

//  size_t AddShardConnectionsWaiting(ShardConfigs const &configs,
//      std::chrono::milliseconds const &timeout = std::chrono::milliseconds(1000));
//
//  void AddShardConnections(
//      ShardConfigs const &configs,
//      std::chrono::milliseconds const &timeout = std::chrono::milliseconds(10000));


//  ClientPtr GetClientForLane(LaneIndex lane);

  uint32_t num_lanes() const;

//  bool IsAlive() const;

  StorageUnitClient &operator=(StorageUnitClient const &) = delete;
  StorageUnitClient &operator=(StorageUnitClient &&) = delete;

protected:


  /// @name Storage Unit Interface
  /// @{
  void AddTransaction(VerifiedTransaction const &tx) override;
  void AddTransactions(TransactionList const &txs) override;

  TxSummaries PollRecentTx(uint32_t max_to_poll) override;

  bool GetTransaction(ConstByteArray const &digest, Transaction &tx) override;
  bool HasTransaction(ConstByteArray const &digest) override;

  Document GetOrCreate(ResourceAddress const &key) override;
  Document Get(ResourceAddress const &key) override;
  void Set(ResourceAddress const &key, StateValue const &value) override;

  // state hash functions
  byte_array::ConstByteArray CurrentHash() override;
  byte_array::ConstByteArray LastCommitHash() override;
  bool                       RevertToHash(Hash const &hash) override;
  byte_array::ConstByteArray Commit() override;
  bool                       HashExists(Hash const &hash) override;
  bool                       Lock(ResourceAddress const &key) override;
  bool                       Unlock(ResourceAddress const &key) override;
  /// @}


private:


  // these will do work for us, it's
  // easier if it has access to our
  // types.
  friend class LaneConnectorWorkerInterface;
  friend class MuddleLaneConnectorWorker;
  friend class LaneConnectorWorker;

  using AddressList               = std::vector<MuddleEndpoint::Address>;
  using Worker                    = MuddleLaneConnectorWorker;
  using WorkerPtr                 = std::shared_ptr<Worker>;
  using BackgroundedWork          = network::BackgroundedWork<Worker>;
  using BackgroundedWorkThread    = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadPtr = std::shared_ptr<BackgroundedWorkThread>;
  using Muddles                   = std::vector<MuddlePtr>;
  using Clients                   = std::vector<ClientPtr>;
  using TxSummaries               = std::vector<TransactionSummary>;
  using MerkleCache               = storage::ObjectStore<crypto::MerkleTree>;
  using MerkleTree                = crypto::MerkleTree;
  using MerkleTreePtr             = std::shared_ptr<MerkleTree>;
  using MerkleStack               = std::vector<MerkleTreePtr>;

//  void WorkCycle();

  Address const &LookupAddress(LaneIndex lane) const;
  Address const &LookupAddress(storage::ResourceID const &resource) const;


//  uint32_t CreateLaneId(LaneIndex lane);

//  std::shared_ptr<Muddle> GetMuddleForLane(LaneIndex lane);

//  void SetLaneLog2(LaneIndex count);

  bool HashInStack(Hash const &hash);

  /// @name Configuration
  /// @{
  AddressList const addresses_;
//  ShardConfigs const shards_;
  uint32_t const                 log2_num_lanes_ = 0;
  /// @}

  /// @name Networking
  /// @{
//  NetworkManager  network_manager_;
//  CertificatePtr  network_identity_;
//  Muddle          muddle_;
  Client          rpc_client_;
  /// @}


//  mutable Mutex             mutex_{__LINE__, __FILE__};
//  LaneToIdentity            lane_to_identity_map_;
//  BackgroundedWork          bg_work_;
//  BackgroundedWorkThreadPtr workthread_;


  // Mutex only needs to protect current merkle and merkle stack, the cache is thread safe
  /// @name State Hash Support
  /// @{
  mutable Mutex merkle_mutex_{__LINE__, __FILE__};
  MerkleTreePtr current_merkle_{};
  MerkleStack   state_merkle_stack_{};
  //  MerkleCache               state_merkle_cache_;
  /// @}
};

inline uint32_t StorageUnitClient::num_lanes() const
{
  return 1u << log2_num_lanes_;
}

}  // namespace ledger
}  // namespace fetch
