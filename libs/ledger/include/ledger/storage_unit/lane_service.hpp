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

#include "core/reactor.hpp"
#include "ledger/shard_config.hpp"
#include "ledger/storage_unit/transaction_storage_engine.hpp"
#include "ledger/storage_unit/transaction_storage_protocol.hpp"

#include "muddle/muddle_interface.hpp"
#include "network/generics/backgrounded_work.hpp"
#include "network/generics/has_worker_thread.hpp"

#include <memory>

namespace fetch {

namespace muddle {
namespace rpc {
class Server;
}
}  // namespace muddle

namespace storage {
class NewRevertibleDocumentStore;

template <typename T>
class TransientObjectStore;

template <typename T>
class ObjectStoreProtocol;

class RevertibleDocumentStoreProtocol;
}  // namespace storage

namespace chain {

class Transaction;

}  // namespace chain

namespace ledger {

class TxFinderProtocol;
class TransactionStoreSyncProtocol;
class TransactionStoreSyncService;
class LaneController;
class LaneControllerProtocol;

class LaneService
{
public:
  static constexpr char const *LOGGING_NAME = "LaneService";

  using MuddlePtr      = muddle::MuddlePtr;
  using CertificatePtr = muddle::ProverPtr;
  using NetworkManager = network::NetworkManager;

  enum class Mode
  {
    CREATE_DATABASE,
    LOAD_DATABASE
  };

  // Construction / Destruction
  explicit LaneService(NetworkManager const &nm, ShardConfig config, Mode mode);
  LaneService(LaneService const &) = delete;
  LaneService(LaneService &&)      = delete;
  ~LaneService();

  // Lane Control
  void StartInternal();
  void StartExternal();
  void StopExternal();
  void StopInternal();

  bool SyncIsReady();

  ShardConfig const &config() const
  {
    return cfg_;
  }

  LaneService &operator=(LaneService const &) = delete;
  LaneService &operator=(LaneService &&) = delete;

private:
  using Reactor                   = core::Reactor;
  using Server                    = fetch::muddle::rpc::Server;
  using ServerPtr                 = std::shared_ptr<Server>;
  using StateDb                   = storage::NewRevertibleDocumentStore;
  using StateDbProto              = storage::RevertibleDocumentStoreProtocol;
  using BackgroundedWork          = network::BackgroundedWork<TransactionStoreSyncService>;
  using BackgroundedWorkThread    = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadPtr = std::shared_ptr<BackgroundedWorkThread>;
  using LaneControllerPtr         = std::shared_ptr<LaneController>;
  using LaneControllerProtocolPtr = std::shared_ptr<LaneControllerProtocol>;
  using StateDbPtr                = std::shared_ptr<StateDb>;
  using StateDbProtoPtr           = std::shared_ptr<StateDbProto>;
  using TxStorePtr                = std::shared_ptr<TransactionStorageEngine>;
  using TxStoreProtoPtr           = std::shared_ptr<TransactionStorageProtocol>;
  using TxSyncProtoPtr            = std::shared_ptr<TransactionStoreSyncProtocol>;
  using TxSyncServicePtr          = std::shared_ptr<TransactionStoreSyncService>;
  using TxFinderProtocolPtr       = std::unique_ptr<TxFinderProtocol>;

  static constexpr uint32_t SYNC_PERIOD_MS = 500;

  TxStorePtr tx_store_;

  Reactor reactor_;

  ShardConfig const         cfg_;
  BackgroundedWork          bg_work_;
  BackgroundedWorkThreadPtr workthread_;

  /// @name External P2P Network
  /// @{
  ServerPtr external_rpc_server_;
  MuddlePtr external_muddle_;  ///< The muddle networking service
  /// @}

  /// @name Internal P2P / Shard Network
  /// @{
  ServerPtr internal_rpc_server_;
  MuddlePtr internal_muddle_;
  /// @}

  /// @name Lane Controller
  /// @{
  LaneControllerPtr         controller_;
  LaneControllerProtocolPtr controller_protocol_;
  /// @}

  /// @name State Database Service
  /// @{
  StateDbPtr      state_db_;
  StateDbProtoPtr state_db_protocol_;
  /// @}

  /// @name Transaction Store
  /// @{
  TxStoreProtoPtr     tx_store_protocol_;
  TxSyncProtoPtr      tx_sync_protocol_;
  TxSyncServicePtr    tx_sync_service_;
  TxFinderProtocolPtr tx_finder_protocol_;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
