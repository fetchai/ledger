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

#include "ledger/storage_unit/lane_service.hpp"
#include "core/service_ids.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include "ledger/storage_unit/lane_controller.hpp"
#include "ledger/storage_unit/lane_controller_protocol.hpp"
#include "ledger/storage_unit/lane_identity.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"
#include "ledger/storage_unit/transaction_store_sync_protocol.hpp"
#include "ledger/storage_unit/transaction_store_sync_service.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/server.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/new_revertible_document_store.hpp"

using fetch::byte_array::ToBase64;

namespace fetch {
namespace ledger {
namespace {

std::string GeneratePrefix(std::string const &storage_path, uint32_t lane)
{
  std::ostringstream oss;
  oss << storage_path << "_lane" << std::setw(3) << std::setfill('0') << lane << "_";
  return oss.str();
}

}  // namespace

LaneService::LaneService(NetworkManager nm, ShardConfig config, bool sign_packets, Mode mode)
  : cfg_{std::move(config)}
{
  // External Muddle network and RPC server
  external_muddle_ =
      std::make_shared<Muddle>(cfg_.external_network_id, cfg_.external_identity, nm, sign_packets);
  external_rpc_server_ =
      std::make_shared<Server>(external_muddle_->AsEndpoint(), SERVICE_LANE, CHANNEL_RPC);

  // Internal muddle network
  internal_muddle_ = std::make_shared<Muddle>(cfg_.internal_network_id, cfg_.internal_identity, nm);
  internal_rpc_server_ =
      std::make_shared<Server>(internal_muddle_->AsEndpoint(), SERVICE_LANE_CTRL, CHANNEL_RPC);

  // Lane Identity + Protocol
  lane_identity_ = std::make_shared<LaneIdentity>(nm, external_muddle_->identity());
  lane_identity_->SetLaneNumber(cfg_.lane_id);
  lane_identity_->SetTotalLanes(cfg_.num_lanes);
  lane_identity_protocol_ = std::make_shared<LaneIdentityProtocol>(*lane_identity_);
  external_rpc_server_->Add(RPC_IDENTITY, lane_identity_protocol_.get());

  // TX Store
  tx_store_ = std::make_shared<TxStore>();

  std::string const prefix = GeneratePrefix(cfg_.storage_path, cfg_.lane_id);
  switch (mode)
  {
  case Mode::CREATE_DATABASE:
    tx_store_->New(prefix + "transaction.db", prefix + "transaction_index.db", true);
    break;
  case Mode::LOAD_DATABASE:
    tx_store_->Load(prefix + "transaction.db", prefix + "transaction_index.db", true);
    break;
  }

  tx_store_protocol_ = std::make_shared<TxStoreProto>(tx_store_.get());
  internal_rpc_server_->Add(RPC_TX_STORE, tx_store_protocol_.get());

  // Controller
  controller_          = std::make_shared<LaneController>(lane_identity_, external_muddle_);
  controller_protocol_ = std::make_shared<LaneControllerProtocol>(controller_.get());
  internal_rpc_server_->Add(RPC_CONTROLLER, controller_protocol_.get());

  tx_sync_protocol_ = std::make_shared<TransactionStoreSyncProtocol>(tx_store_.get(), cfg_.lane_id);

  // prepare the sync config
  TransactionStoreSyncService::Config sync_cfg{};
  sync_cfg.lane_id                    = cfg_.lane_id;
  sync_cfg.verification_threads       = cfg_.verification_threads;
  sync_cfg.main_timeout               = cfg_.sync_service_timeout;
  sync_cfg.promise_wait_timeout       = cfg_.sync_service_promise_timeout;
  sync_cfg.fetch_object_wait_duration = cfg_.sync_service_fetch_period;

  tx_sync_service_ =
      std::make_shared<TransactionStoreSyncService>(sync_cfg, external_muddle_, tx_store_);

  tx_store_->SetCallback([this](VerifiedTransaction const &tx) { tx_sync_protocol_->OnNewTx(tx); });

  tx_sync_service_->SetTrimCacheCallback([this]() { tx_sync_protocol_->TrimCache(); });

  // TX Sync protocol
  external_rpc_server_->Add(RPC_TX_STORE_SYNC, tx_sync_protocol_.get());

  // State DB
  state_db_ = std::make_shared<StateDb>();
  switch (mode)
  {
  case Mode::CREATE_DATABASE:
    state_db_->New(prefix + "state.db", prefix + "state_deltas.db", prefix + "state_index.db",
                   prefix + "state_index_deltas.db", false);
    break;
  case Mode::LOAD_DATABASE:
    state_db_->Load(prefix + "state.db", prefix + "state_deltas.db", prefix + "state_index.db",
                    prefix + "state_index_deltas.db", true);
    break;
  }

  state_db_protocol_ =
      std::make_shared<StateDbProto>(state_db_.get(), cfg_.lane_id, cfg_.num_lanes);
  internal_rpc_server_->Add(RPC_STATE, state_db_protocol_.get());

  FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, " Initialised.");
}

LaneService::~LaneService()
{
  workthread_ = nullptr;

  FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, " Teardown.");

  external_muddle_->Shutdown();
  internal_muddle_->Shutdown();
  tx_sync_service_.reset();

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

void LaneService::Start()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Establishing Lane ", cfg_.lane_id,
                 " Service on tcp://0.0.0.0:", cfg_.external_port,
                 " ID: ", ToBase64(cfg_.external_identity->identity().identifier()));
  FETCH_LOG_INFO(LOGGING_NAME, "Establishing Lane ", cfg_.lane_id,
                 " Service on tcp://127.0.0.1:", cfg_.internal_port,
                 " ID: ", ToBase64(cfg_.internal_identity->identity().identifier()));

  external_muddle_->Start({cfg_.external_port});
  internal_muddle_->Start({cfg_.internal_port});

  tx_sync_service_->Start();

  // TX Sync service
  workthread_ = std::make_shared<BackgroundedWorkThread>(
      &bg_work_, "BW:LS-" + std::to_string(cfg_.lane_id), [this]() { tx_sync_service_->Work(); });
  workthread_->ChangeWaitTime(std::chrono::milliseconds{unsigned{SYNC_PERIOD_MS}});
}

void LaneService::Stop()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Lane ", cfg_.lane_id, " Stopping.");
  tx_sync_service_->Stop();
  workthread_ = nullptr;

  external_muddle_->Stop();
  internal_muddle_->Stop();
}

bool LaneService::SyncIsReady()
{
  return tx_sync_service_->IsReady();
}

}  // namespace ledger
}  // namespace fetch
