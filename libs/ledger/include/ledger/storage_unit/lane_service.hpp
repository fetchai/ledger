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
#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_controller.hpp"
#include "ledger/storage_unit/lane_controller_protocol.hpp"
#include "ledger/storage_unit/lane_identity.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"
#include "ledger/storage_unit/transaction_store_sync_protocol.hpp"
#include "ledger/storage_unit/transaction_store_sync_service.hpp"
#include "network/details/thread_pool.hpp"
#include "network/generics/backgrounded_work.hpp"
#include "network/generics/has_worker_thread.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/service/server.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/object_store.hpp"
#include "storage/object_store_protocol.hpp"
#include "storage/revertible_document_store.hpp"
#include "storage/transient_object_store.hpp"

#include <iomanip>
#include <memory>

namespace fetch {
namespace ledger {

class LaneService
{
public:
  using Muddle         = muddle::Muddle;
  using MuddlePtr      = std::shared_ptr<Muddle>;
  using Server         = fetch::muddle::rpc::Server;
  using ServerPtr      = std::shared_ptr<Server>;
  using CertificatePtr = Muddle::CertificatePtr;
  using NetworkId      = muddle::Muddle::NetworkId;

  using DocumentStore             = storage::NewRevertibleDocumentStore;
  using DocumentStoreProtocol     = storage::RevertibleDocumentStoreProtocol;
  using TransactionStore          = storage::TransientObjectStore<VerifiedTransaction>;
  using TransactionStoreProtocol  = storage::ObjectStoreProtocol<VerifiedTransaction>;
  using BackgroundedWork          = network::BackgroundedWork<TransactionStoreSyncService>;
  using BackgroundedWorkThread    = network::HasWorkerThread<BackgroundedWork>;
  using BackgroundedWorkThreadPtr = std::shared_ptr<BackgroundedWorkThread>;

  using Identifier = byte_array::ConstByteArray;

  static constexpr char const *LOGGING_NAME = "LaneService";

  // TODO(issue 7): Make config JSON
  LaneService(
      std::string const &storage_path, uint32_t const &lane, uint32_t const &total_lanes,
      uint16_t port, NetworkId network_id, fetch::network::NetworkManager tm,
      std::size_t verification_threads, bool refresh_storage = false,
      std::chrono::milliseconds sync_service_timeout         = std::chrono::milliseconds(5000),
      std::chrono::milliseconds sync_service_promise_timeout = std::chrono::milliseconds(2000),
      std::chrono::milliseconds sync_service_fetch_period    = std::chrono::milliseconds(5000))
    : port_(port)
    , lane_(lane)
  {
    std::unique_ptr<crypto::Prover> certificate_ = std::make_unique<crypto::ECDSASigner>();

    lane_identity_ = std::make_shared<LaneIdentity>(tm, certificate_->identity());
    muddle_        = std::make_shared<Muddle>(network_id, std::move(certificate_), tm);
    server_        = std::make_shared<Server>(muddle_->AsEndpoint(), SERVICE_LANE, CHANNEL_RPC);

    // format and generate the prefix
    std::string prefix;
    {
      std::stringstream s;
      s << storage_path;
      s << "_lane" << std::setw(3) << std::setfill('0') << lane << "_";
      prefix = s.str();
    }

    // Lane Identity
    lane_identity_->SetLaneNumber(lane);
    lane_identity_->SetTotalLanes(total_lanes);
    lane_identity_protocol_ = std::make_unique<LaneIdentityProtocol>(lane_identity_.get());
    server_->Add(RPC_IDENTITY, lane_identity_protocol_.get());

    // TX Store
    tx_store_ = std::make_shared<TransactionStore>();
    if (refresh_storage)
    {
      tx_store_->New(prefix + "transaction.db", prefix + "transaction_index.db", true);
    }
    else
    {
      tx_store_->Load(prefix + "transaction.db", prefix + "transaction_index.db", true);
    }

    tx_store_protocol_ = std::make_unique<TransactionStoreProtocol>(tx_store_.get());
    server_->Add(RPC_TX_STORE, tx_store_protocol_.get());

    // Controller
    controller_          = std::make_shared<LaneController>(lane_identity_, muddle_);
    controller_protocol_ = std::make_unique<LaneControllerProtocol>(controller_.get());
    server_->Add(RPC_CONTROLLER, controller_protocol_.get());

    tx_sync_protocol_ = std::make_unique<TransactionStoreSyncProtocol>(tx_store_.get(), lane_);
    tx_sync_service_  = std::make_shared<TransactionStoreSyncService>(
        lane_, muddle_, tx_store_, controller_, verification_threads, sync_service_timeout,
        sync_service_promise_timeout, sync_service_fetch_period);

    tx_store_->SetCallback(
        [this](VerifiedTransaction const &tx) { tx_sync_protocol_->OnNewTx(tx); });

    tx_sync_service_->SetTrimCacheCallback([this]() { tx_sync_protocol_->TrimCache(); });

    // TX Sync protocol
    server_->Add(RPC_TX_STORE_SYNC, tx_sync_protocol_.get());

    // State DB
    state_db_ = std::make_unique<DocumentStore>();
    if (refresh_storage)
    {
      state_db_->New(prefix + "state.db", prefix + "state_deltas.db", prefix + "state_index.db",
                     prefix + "state_index_deltas.db", false);
    }
    else
    {
      state_db_->Load(prefix + "state.db", prefix + "state_deltas.db", prefix + "state_index.db",
                      prefix + "state_index_deltas.db", true);
    }

    state_db_protocol_ =
        std::make_unique<DocumentStoreProtocol>(state_db_.get(), lane, total_lanes);
    server_->Add(RPC_STATE, state_db_protocol_.get());

    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", lane_, " Initialised.");
  }

  virtual ~LaneService()
  {
    workthread_ = nullptr;

    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", lane_, " Teardown.");

    muddle_->Shutdown();
    tx_sync_service_ = nullptr;

    lane_identity_protocol_.reset();
    lane_identity_.reset();

    // TODO(issue 24): Remove protocol
    state_db_protocol_.reset();
    state_db_.reset();

    // TODO(issue 24): Remove protocol
    tx_store_protocol_.reset();
    tx_store_.reset();

    tx_sync_protocol_.reset();

    // TODO(issue 24): Remove protocol
    controller_protocol_.reset();
    controller_.reset();
  }

  void Start()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Establishing Lane ", lane_, " Service on tcp://127.0.0.1:", port_,
                   " ID: ", byte_array::ToBase64(lane_identity_->Identity().identifier()));
    muddle_->Start({port_});

    tx_sync_service_->Start();

    // TX Sync service
    workthread_ = std::make_shared<BackgroundedWorkThread>(
        &bg_work_, "BW:LS-" + std::to_string(lane_), [this]() { tx_sync_service_->Work(); });
    workthread_->ChangeWaitTime(std::chrono::milliseconds{unsigned{SYNC_PERIOD_MS}});
  }

  void Stop()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Lane ", lane_, " Stopping.");
    tx_sync_service_->Stop();
    workthread_ = nullptr;
    muddle_->Stop();
  }

  bool SyncIsReady()
  {
    return tx_sync_service_->IsReady();
  }

private:
  std::shared_ptr<LaneIdentity>         lane_identity_;
  std::unique_ptr<LaneIdentityProtocol> lane_identity_protocol_;

  std::shared_ptr<LaneController>         controller_;
  std::unique_ptr<LaneControllerProtocol> controller_protocol_;

  std::unique_ptr<DocumentStore>         state_db_;
  std::unique_ptr<DocumentStoreProtocol> state_db_protocol_;

  std::shared_ptr<TransactionStore>         tx_store_;
  std::unique_ptr<TransactionStoreProtocol> tx_store_protocol_;

  std::unique_ptr<TransactionStoreSyncProtocol> tx_sync_protocol_;
  std::shared_ptr<TransactionStoreSyncService>  tx_sync_service_;

  ServerPtr server_;
  MuddlePtr muddle_;  ///< The muddle networking service
  uint16_t  port_;

  BackgroundedWork          bg_work_;
  BackgroundedWorkThreadPtr workthread_;

  uint32_t lane_;

  static constexpr unsigned int SYNC_PERIOD_MS = 500;
};

}  // namespace ledger
}  // namespace fetch
