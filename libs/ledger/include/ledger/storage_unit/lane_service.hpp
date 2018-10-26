#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
#include "network/details/thread_pool.hpp"
#include "network/management/connection_register.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/service/server.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/object_store.hpp"
#include "storage/object_store_protocol.hpp"
#include "storage/object_store_syncronisation_protocol.hpp"
#include "storage/revertible_document_store.hpp"

#include <iomanip>
#include <memory>

namespace fetch {
namespace ledger {

class LaneService  //: public service::ServiceServer<fetch::network::TCPServer>
{
public:
  using Muddle         = muddle::Muddle;
  using MuddlePtr      = std::shared_ptr<Muddle>;
  using Server         = fetch::muddle::rpc::Server;
  using ServerPtr      = std::shared_ptr<Server>;
  using CertificatePtr = Muddle::CertificatePtr;  // == std::unique_ptr<crypto::Prover>;

  using connectivity_details_type    = LaneConnectivityDetails;
  using document_store_type          = storage::RevertibleDocumentStore;
  using DocumentStoreProtocolImpl = storage::RevertibleDocumentStoreProtocol;
  using transaction_store_type       = storage::ObjectStore<fetch::chain::VerifiedTransaction>;
  using transaction_store_protocol_type =
      storage::ObjectStoreProtocol<fetch::chain::VerifiedTransaction>;
  using client_register_type     = fetch::network::ConnectionRegister<connectivity_details_type>;
  using controller_type          = LaneController;
  using controller_protocol_type = LaneControllerProtocol;
  using connection_handle_type   = client_register_type::connection_handle_type;
  using super_type               = service::ServiceServer<fetch::network::TCPServer>;
  using tx_sync_protocol_type    = storage::ObjectStoreSyncronisationProtocol<
      client_register_type, fetch::chain::VerifiedTransaction, fetch::chain::UnverifiedTransaction>;
  using thread_pool_type = network::ThreadPool;

  using Identifier = byte_array::ConstByteArray;

  static constexpr char const *LOGGING_NAME = "LaneService";

  // TODO(issue 7): Make config JSON
  LaneService(std::string const &storage_path, uint32_t const &lane, uint32_t const &total_lanes,
              uint16_t port, fetch::network::NetworkManager tm, bool refresh_storage = false)
  {

    thread_pool_ = network::MakeThreadPool(1);
    port_        = port;
    lane_        = lane;

    // Setting lane certificate up
    // TODO(issue 24): Load from somewhere
    crypto::ECDSASigner *certificate = new crypto::ECDSASigner();
    certificate->GenerateKeys();

    std::unique_ptr<crypto::Prover> certificate_;
    certificate_.reset(certificate);

    lane_identity_ = std::make_shared<LaneIdentity>(register_, tm, certificate_->identity());
    muddle_        = std::make_shared<Muddle>(std::move(certificate_), tm);
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
    tx_store_ = std::make_unique<transaction_store_type>();
    if (refresh_storage)
    {
      tx_store_->New(prefix + "transaction.db", prefix + "transaction_index.db", true);
    }
    else
    {
      tx_store_->Load(prefix + "transaction.db", prefix + "transaction_index.db", true);
    }

    tx_sync_protocol_  = std::make_unique<tx_sync_protocol_type>(RPC_TX_STORE_SYNC, register_,
                                                                thread_pool_, tx_store_.get());
    tx_store_protocol_ = std::make_unique<transaction_store_protocol_type>(tx_store_.get());
    tx_store_protocol_->OnSetObject(
        [this](fetch::chain::VerifiedTransaction const &tx) { tx_sync_protocol_->AddToCache(tx); });

    server_->Add(RPC_TX_STORE, tx_store_protocol_.get());

    // TX Sync
    server_->Add(RPC_TX_STORE_SYNC, tx_sync_protocol_.get());

    // State DB
    state_db_ = std::make_unique<document_store_type>();
    if (refresh_storage)
    {
      state_db_->New(prefix + "state.db", prefix + "state_deltas.db", prefix + "state_index.db",
                     prefix + "state_index_deltas.db");
    }
    else
    {
      state_db_->Load(prefix + "state.db", prefix + "state_deltas.db", prefix + "state_index.db",
                      prefix + "state_index_deltas.db", true);
    }

    state_db_protocol_ =
      std::make_unique<DocumentStoreProtocolImpl>(state_db_.get(), lane, total_lanes);
    server_->Add(RPC_STATE, state_db_protocol_.get());

    // Controller
    controller_ = std::make_unique<controller_type>(RPC_IDENTITY, lane_identity_, register_, tm);
    controller_protocol_ = std::make_unique<controller_protocol_type>(controller_.get());
    server_->Add(RPC_CONTROLLER, controller_protocol_.get());
  }

  ~LaneService()
  {
    thread_pool_->Stop();

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
    FETCH_LOG_INFO(LOGGING_NAME, "Establishing Lane ", lane_, " Service on rpc://127.0.0.1:", port_,
                   " ID: ", byte_array::ToBase64(lane_identity_->Identity().identifier()));
    muddle_->Start({port_});

    thread_pool_->Start();
    tx_sync_protocol_->Start();
  }

  void Stop()
  {
    tx_sync_protocol_->Stop();
    thread_pool_->Stop();
    muddle_->Stop();
  }

private:
  client_register_type register_;

  std::shared_ptr<LaneIdentity>         lane_identity_;
  std::unique_ptr<LaneIdentityProtocol> lane_identity_protocol_;

  std::unique_ptr<controller_type>          controller_;
  std::unique_ptr<controller_protocol_type> controller_protocol_;

  std::unique_ptr<document_store_type>          state_db_;
  std::unique_ptr<DocumentStoreProtocolImpl> state_db_protocol_;

  std::unique_ptr<transaction_store_type>          tx_store_;
  std::unique_ptr<transaction_store_protocol_type> tx_store_protocol_;

  std::unique_ptr<tx_sync_protocol_type> tx_sync_protocol_;
  thread_pool_type                       thread_pool_;

  ServerPtr    server_;
  MuddlePtr    muddle_;  ///< The muddle networking service
  mutex::Mutex certificate_lock_{__LINE__, __FILE__};
  uint16_t     port_;
  uint32_t     lane_;
};

}  // namespace ledger
}  // namespace fetch
