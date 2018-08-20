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

class LaneService : public service::ServiceServer<fetch::network::TCPServer>
{
public:
  using connectivity_details_type    = LaneConnectivityDetails;
  using document_store_type          = storage::RevertibleDocumentStore;
  using document_store_protocol_type = storage::RevertibleDocumentStoreProtocol;
  using transaction_store_type       = storage::ObjectStore<fetch::chain::VerifiedTransaction>;
  using transaction_store_protocol_type =
      storage::ObjectStoreProtocol<fetch::chain::VerifiedTransaction>;
  using client_register_type     = fetch::network::ConnectionRegister<connectivity_details_type>;
  using controller_type          = LaneController;
  using controller_protocol_type = LaneControllerProtocol;
  using identity_type            = LaneIdentity;
  using identity_protocol_type   = LaneIdentityProtocol;
  using connection_handle_type   = client_register_type::connection_handle_type;
  using super_type               = service::ServiceServer<fetch::network::TCPServer>;
  using tx_sync_protocol_type    = storage::ObjectStoreSyncronisationProtocol<
      client_register_type, fetch::chain::VerifiedTransaction, fetch::chain::UnverifiedTransaction>;
  using thread_pool_type = network::ThreadPool;

  enum
  {
    IDENTITY = 1,
    STATE,
    TX_STORE,
    TX_STORE_SYNC,
    SLICE_STORE,
    SLICE_STORE_SYNC,
    CONTROLLER
  };

  // TODO(issue 7): Make config JSON
  LaneService(std::string const &db_dir, uint32_t const &lane, uint32_t const &total_lanes,
              uint16_t port, fetch::network::NetworkManager tm, bool start_sync = true)
    : super_type(port, tm)
  {

    this->SetConnectionRegister(register_);

    fetch::logger.Warn("Establishing Lane ", lane, " Service on rpc://127.0.0.1:", port);
    thread_pool_ = network::MakeThreadPool(1);

    // Setting lane certificate up
    // TODO(issue 24): Load from somewhere
    crypto::ECDSASigner *certificate = new crypto::ECDSASigner();
    certificate->GenerateKeys();
    certificate_.reset(certificate);

    // format and generate the prefix
    std::string prefix;
    {
      std::stringstream s;
      s << db_dir;
      s << "_lane" << std::setw(3) << std::setfill('0') << lane << "_";
      prefix = s.str();
    }

    // Lane Identity
    identity_ = std::make_shared<identity_type>(register_, tm, certificate_->identity());
    identity_->SetLaneNumber(lane);
    identity_->SetTotalLanes(total_lanes);
    identity_protocol_ = std::make_unique<identity_protocol_type>(identity_.get());
    this->Add(IDENTITY, identity_protocol_.get());

    // TX Store
    tx_store_ = std::make_unique<transaction_store_type>();
    tx_store_->Load(prefix + "transaction.db", prefix + "transaction_index.db", true);

    tx_sync_protocol_  = std::make_unique<tx_sync_protocol_type>(TX_STORE_SYNC, register_,
                                                                thread_pool_, tx_store_.get());
    tx_store_protocol_ = std::make_unique<transaction_store_protocol_type>(tx_store_.get());
    tx_store_protocol_->OnSetObject(
        [this](fetch::chain::VerifiedTransaction const &tx) { tx_sync_protocol_->AddToCache(tx); });

    this->Add(TX_STORE, tx_store_protocol_.get());

    // TX Sync
    this->Add(TX_STORE_SYNC, tx_sync_protocol_.get());

    // State DB
    state_db_ = std::make_unique<document_store_type>();
    state_db_->Load(prefix + "state.db", prefix + "state_deltas.db", prefix + "state_index.db",
                    prefix + "state_index_deltas.db", true);

    state_db_protocol_ =
        std::make_unique<document_store_protocol_type>(state_db_.get(), lane, total_lanes);
    this->Add(STATE, state_db_protocol_.get());

    // Controller
    controller_          = std::make_unique<controller_type>(IDENTITY, identity_, register_, tm);
    controller_protocol_ = std::make_unique<controller_protocol_type>(controller_.get());
    this->Add(CONTROLLER, controller_protocol_.get());

    thread_pool_->Start();

    if (start_sync)
    {
      tx_sync_protocol_->Start();
    }
  }

  ~LaneService()
  {
    identity_protocol_.reset();
    identity_.reset();

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

private:
  client_register_type register_;

  std::shared_ptr<identity_type>          identity_;
  std::unique_ptr<identity_protocol_type> identity_protocol_;

  std::unique_ptr<controller_type>          controller_;
  std::unique_ptr<controller_protocol_type> controller_protocol_;

  std::unique_ptr<document_store_type>          state_db_;
  std::unique_ptr<document_store_protocol_type> state_db_protocol_;

  std::unique_ptr<transaction_store_type>          tx_store_;
  std::unique_ptr<transaction_store_protocol_type> tx_store_protocol_;

  std::unique_ptr<tx_sync_protocol_type> tx_sync_protocol_;
  thread_pool_type                       thread_pool_;

  mutex::Mutex                    certificate_lock_;
  std::unique_ptr<crypto::Prover> certificate_;
};

}  // namespace ledger
}  // namespace fetch
