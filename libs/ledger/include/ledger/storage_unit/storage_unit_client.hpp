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
  struct ClientDetails
  {
    crypto::Identity      identity;
    std::atomic<uint32_t> lane{0};
  };

  using Client    = muddle::rpc::Client;
  using ClientPtr = std::shared_ptr<Client>;
  using MuddleEp  = muddle::MuddleEndpoint;
  using Muddle    = muddle::Muddle;
  using MuddlePtr = std::shared_ptr<Muddle>;
  using Address   = Muddle::Address;
  using Uri       = Muddle::Uri;
  using Peer      = fetch::network::Peer;

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

  explicit StorageUnitClient(NetworkManager const &tm)
    : network_manager_(tm)
  {
    //    state_merkle_cache_.Load("merkle_hash_cache_index.db", "merkle_hash_cache.db", true);
  }

  StorageUnitClient(StorageUnitClient const &) = delete;
  StorageUnitClient(StorageUnitClient &&)      = delete;
  StorageUnitClient &operator=(StorageUnitClient const &) = delete;
  StorageUnitClient &operator=(StorageUnitClient &&) = delete;

  void SetNumberOfLanes(LaneIndex count)
  {
    SetLaneLog2(count);
    muddles_.resize(count);
    clients_.resize(count);
    assert(count == (1u << log2_lanes_));
  }

  size_t AddLaneConnectionsWaiting(
      std::map<LaneIndex, Uri> const & lanes,
      std::chrono::milliseconds const &timeout = std::chrono::milliseconds(1000));

  void AddLaneConnections(
      std::map<LaneIndex, Uri> const & lanes,
      std::chrono::milliseconds const &timeout = std::chrono::milliseconds(10000));

  void GetAddressForLane(LaneIndex lane, Address &address) const
  {
    FETCH_LOCK(mutex_);

    auto iter = lane_to_identity_map_.find(lane);
    if (iter != lane_to_identity_map_.end())
    {
      address = iter->second;
      return;
    }
    throw std::runtime_error("Could not get address for lane " + std::to_string(lane));
  }

  ClientPtr GetClientForLane(LaneIndex lane)
  {
    auto muddle = GetMuddleForLane(lane);
    if (!clients_[lane])
    {
      clients_[lane] =
          std::make_shared<Client>("R:SU-" + std::to_string(lane), muddle->AsEndpoint(),
                                   Muddle::Address(), SERVICE_LANE, CHANNEL_RPC);
    }
    return clients_[lane];
  }

  void AddTransaction(VerifiedTransaction const &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<Transaction>;

    auto      res  = fetch::storage::ResourceID(tx.digest());
    LaneIndex lane = res.lane(log2_lanes_);

    Address address;
    GetAddressForLane(lane, address);
    auto client  = GetClientForLane(lane);
    auto promise = client->CallSpecificAddress(address, RPC_TX_STORE, protocol::SET, res, tx);
    FETCH_LOG_PROMISE();
    promise->Wait();
  }

  void AddTransactions(TransactionList const &txs) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<Transaction>;

    std::vector<protocol::ElementList> transaction_lists(1 << log2_lanes_);

    for (auto const &tx : txs)
    {
      // determine the lane for this given transaction
      auto      res  = fetch::storage::ResourceID(tx.digest());
      LaneIndex lane = res.lane(log2_lanes_);

      transaction_lists.at(lane).push_back({res, tx});
    }

    std::vector<Promise> promises;
    promises.reserve(1 << log2_lanes_);

    // dispatch all the set requests off
    {
      LaneIndex lane{0};
      for (auto const &list : transaction_lists)
      {
        Address address;
        GetAddressForLane(lane, address);
        auto client = GetClientForLane(lane);
        if (!list.empty())
        {
          promises.emplace_back(
              client->CallSpecificAddress(address, RPC_TX_STORE, protocol::SET_BULK, list));
        }

        ++lane;
      }
    }

    // wait for all the requests to complete
    for (auto &promise : promises)
    {
      promise->Wait();
    }
  }

  TxSummaries PollRecentTx(uint32_t max_to_poll) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<Transaction>;
    std::vector<service::Promise> promises;
    TxSummaries                   new_txs;

    FETCH_LOG_DEBUG(LOGGING_NAME, "Polling recent transactions from lanes");

    // Assume that the lanes are roughly balanced in terms of new TXs
    for (auto const &lane_index : lane_to_identity_map_)
    {
      auto    lane = lane_index.first;
      Address address;
      GetAddressForLane(lane, address);
      auto client = GetClientForLane(lane);
      auto promise =
          client->CallSpecificAddress(address, RPC_TX_STORE, protocol::GET_RECENT,
                                      uint32_t(max_to_poll / lane_to_identity_map_.size()));
      FETCH_LOG_PROMISE();
      promises.push_back(promise);
    }

    for (auto const &promise : promises)
    {
      auto txs = promise->As<TxSummaries>();

      new_txs.insert(new_txs.end(), std::make_move_iterator(txs.begin()),
                     std::make_move_iterator(txs.end()));
    }

    if (new_txs.size() > 0)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Found: ", new_txs.size(), " newly seen TXs from lanes");
    }
    return new_txs;
  }

  bool GetTransaction(byte_array::ConstByteArray const &digest, Transaction &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<Transaction>;

    auto      res  = fetch::storage::ResourceID(digest);
    LaneIndex lane = res.lane(log2_lanes_);
    Address   address;

    try
    {
      GetAddressForLane(lane, address);

      auto client  = GetClientForLane(lane);
      auto promise = client->CallSpecificAddress(address, RPC_TX_STORE, protocol::GET, res);
      tx           = promise->As<VerifiedTransaction>();
    }
    catch (std::exception const &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to get transaction, because: ", e.what());
      return false;
    }

    return true;
  }

  // TODO(HUT): comment these doxygen-style
  Document GetOrCreate(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    try
    {
      GetAddressForLane(lane, address);
    }
    catch (std::runtime_error &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to get or create document, because: ", e.what());
      Document doc;
      doc.failed = true;
      return doc;
    }
    auto client  = GetClientForLane(lane);
    auto promise = client->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET_OR_CREATE,
        key.as_resource_id());

    return promise->As<storage::Document>();
  }

  Document Get(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    try
    {
      GetAddressForLane(lane, address);
    }
    catch (std::runtime_error &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to get document, because: ", e.what());
      Document doc;
      doc.failed = true;
      return doc;
    }

    auto client  = GetClientForLane(lane);
    auto promise = client->CallSpecificAddress(address, RPC_STATE,
                                               fetch::storage::RevertibleDocumentStoreProtocol::GET,
                                               key.as_resource_id());

    return promise->As<storage::Document>();
  }

  void Set(ResourceAddress const &key, StateValue const &value) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    try
    {
      GetAddressForLane(lane, address);
    }
    catch (std::runtime_error &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to call SET (store document), because: ", e.what());
      return;
    }
    auto client  = GetClientForLane(lane);
    auto promise = client->CallSpecificAddress(address, RPC_STATE,
                                               fetch::storage::RevertibleDocumentStoreProtocol::SET,
                                               key.as_resource_id(), value);

    FETCH_LOG_PROMISE();
    promise->Wait();  // TODO(HUT): no error state is checked/allowed for setting things?
  }

  // state hash functions
  byte_array::ConstByteArray CurrentHash() override;
  byte_array::ConstByteArray LastCommitHash() override;
  bool                       RevertToHash(Hash const &hash) override;
  byte_array::ConstByteArray Commit() override;
  bool                       HashExists(Hash const &hash) override;

  bool Lock(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    try
    {
      GetAddressForLane(lane, address);
    }
    catch (std::runtime_error &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to call Lock, because: ", e.what());
      return false;
    }
    auto client  = GetClientForLane(lane);
    auto promise = client->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::LOCK,
        key.as_resource_id());

    return promise->As<bool>();
  }

  bool Unlock(ResourceAddress const &key) override
  {
    LaneIndex lane = key.lane(log2_lanes_);

    Address address;
    try
    {
      GetAddressForLane(lane, address);
    }
    catch (std::runtime_error &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to call Unlock, because: ", e.what());
      return false;
    }
    auto client  = GetClientForLane(lane);
    auto promise = client->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK,
        key.as_resource_id());

    return promise->As<bool>();
  }

  std::size_t lanes() const
  {
    return lane_to_identity_map_.size();
  }

  bool IsAlive() const
  {
    return true;
  }

private:
  // these will do work for us, it's
  // easier if it has access to our
  // types.
  friend class LaneConnectorWorkerInterface;
  friend class MuddleLaneConnectorWorker;
  friend class LaneConnectorWorker;

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

  void WorkCycle();

  std::shared_ptr<Muddle> GetMuddleForLane(LaneIndex lane)
  {
    if (!muddles_[lane])
    {
      muddles_[lane] = Muddle::CreateMuddle(Muddle::NetworkId("Storage" + std::to_string(lane)),
                                            network_manager_);
      muddles_[lane]->Start({});
    }
    return muddles_[lane];
  }

  void SetLaneLog2(LaneIndex count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  bool HashInStack(Hash const &hash);

  NetworkManager            network_manager_;
  uint32_t                  log2_lanes_ = 0;
  mutable Mutex             mutex_{__LINE__, __FILE__};
  LaneToIdentity            lane_to_identity_map_;
  BackgroundedWork          bg_work_;
  BackgroundedWorkThreadPtr workthread_;
  Muddles                   muddles_;
  Clients                   clients_;

  // Mutex only needs to protect current merkle and merkle stack, the cache is thread safe
  mutable Mutex merkle_mutex_{__LINE__, __FILE__};
  MerkleTreePtr current_merkle_;
  MerkleStack   state_merkle_stack_;
  //  MerkleCache               state_merkle_cache_;
};

}  // namespace ledger
}  // namespace fetch
