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

#include <memory>

#include "crypto/ecdsa.hpp"
#include "crypto/prover.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/main_chain_controller.hpp"
#include "ledger/chain/main_chain_controller_protocol.hpp"
#include "ledger/chain/main_chain_details.hpp"
#include "ledger/chain/main_chain_identity.hpp"
#include "ledger/chain/main_chain_identity_protocol.hpp"
#include "ledger/chain/main_chain_protocol.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/server.hpp"
#include "storage/object_store.hpp"
#include "storage/object_store_protocol.hpp"
#include "storage/object_store_syncronisation_protocol.hpp"

#include "network/muddle/rpc/server.hpp"

namespace fetch {
namespace chain {

class MainChainService : public service::ServiceServer<fetch::network::TCPServer>
{
public:
  using ProofType = fetch::chain::MainChain::ProofType;
  using BlockType = fetch::chain::MainChain::BlockType;
  using body_type = fetch::chain::MainChain::BlockType::body_type;
  using BlockHash = fetch::chain::MainChain::BlockHash;

  using connectivity_details_type = MainChainDetails;
  using client_register_type      = fetch::network::ConnectionRegister<connectivity_details_type>;

  using mainchain_type          = fetch::chain::MainChain;
  using mainchain_protocol_type = fetch::chain::MainChainProtocol<client_register_type>;

  using block_store_type          = storage::ObjectStore<BlockType>;
  using block_store_protocol_type = storage::ObjectStoreProtocol<BlockType>;
  using block_sync_protocol_type =
      storage::ObjectStoreSyncronisationProtocol<client_register_type, BlockType>;

  using controller_type          = MainChainController;
  using controller_protocol_type = MainChainControllerProtocol;

  using identity_controller_type = MainChainIdentity;
  using identity_protocol_type   = MainChainIdentityProtocol;
  using connection_handle_type   = client_register_type::connection_handle_type;
  using super_type               = service::ServiceServer<fetch::network::TCPServer>;

  using thread_pool_type = network::ThreadPool;

  static constexpr char const *LOGGING_NAME = "MainChainService";

  enum
  {
    IDENTITY = 1,
    CHAIN,
    CONTROLLER
  };

  MainChainService(std::string const &db_dir, uint16_t port, fetch::network::NetworkManager tm,
                   const std::string &identifier, bool start_sync = true)
    : super_type(port, tm)
  {

    FETCH_LOG_WARN(LOGGING_NAME,"Establishing mainchain Service on rpc://127.0.0.1:", port);

    my_details_.Make();

    thread_pool_ = network::MakeThreadPool(1);

    // format and generate the prefix
    std::string prefix;
    {
      std::stringstream s;
      s << db_dir;
      s << "_mainchain" << std::setw(3) << std::setfill('0') << "_";
      prefix = s.str();
    }

    // Main chain Identity
    identity_controller_ = std::make_shared<identity_controller_type>(register_, tm, my_details_);
    identity_protocol_   = std::make_unique<identity_protocol_type>(identity_controller_.get());
    this->Add(IDENTITY, identity_protocol_.get());

    // Setting mainchain certificate up
    // TODO(tfr): Load from somewhere
    crypto::ECDSASigner *certificate = new crypto::ECDSASigner();
    certificate->GenerateKeys();
    certificate_.reset(certificate);

    {
      auto deets      = my_details_.Lock();
      deets->identity = certificate_->identity();
      deets->Sign(certificate_.get());

      // TODO: ECDSA verifier broke
      // crypto::ECDSAVerifier verifier(certificate->identity());
      // if(!my_details_->details.Verify(&verifier) ) {
      //  TODO_FAIL("Could not verify own identity");
      //}
    }

    mainchain_          = std::make_unique<mainchain_type>();
    mainchain_protocol_ = std::make_unique<mainchain_protocol_type>(CHAIN, register_, thread_pool_,
                                                                    identifier, mainchain_.get());

    this->Add(CHAIN, mainchain_protocol_.get());

    controller_ = std::make_unique<controller_type>(IDENTITY, identity_controller_, register_, tm,
                                                    my_details_, mainchain_protocol_);
    controller_protocol_ = std::make_unique<controller_protocol_type>(controller_.get());
    this->Add(CONTROLLER, controller_protocol_.get());

  }

  void Start()
  {
    TCPServer::Start();
    thread_pool_->Start();
    mainchain_protocol_->Start();
  }

  void Stop()
  {
    thread_pool_->Stop();
    mainchain_protocol_->Stop();
    TCPServer::Stop();
  }

  void SetOwnerIdentity(const crypto::Identity &identity)
  {
    auto deets                               = my_details_.Lock();
    deets->owning_discovery_service_identity = identity;
  }

  std::string GetOwnerIdentityString() { return my_details_.Lock()->GetOwnerIdentityString(); }

  ~MainChainService()
  {

    identity_protocol_.reset();
    identity_controller_.reset();
  }

  void ConnectionDropped(fetch::network::TCPClient::handle_type connection_handle)
  {
    mainchain_protocol_->ConnectionDropped(connection_handle);
  }

  void PublishBlock(const chain::MainChain::BlockType &blk)
  {
    mainchain_protocol_->PublishBlock(blk);
  }

  mainchain_type *         mainchain() { return mainchain_.get(); }
  mainchain_protocol_type *mainchainprotocol() { return mainchain_protocol_.get(); }

private:
  client_register_type register_;
  thread_pool_type     thread_pool_;

  std::shared_ptr<identity_controller_type> identity_controller_;
  std::unique_ptr<identity_protocol_type>   identity_protocol_;

  std::unique_ptr<mainchain_type>          mainchain_;
  std::shared_ptr<mainchain_protocol_type> mainchain_protocol_;

  std::unique_ptr<controller_type>          controller_;
  std::unique_ptr<controller_protocol_type> controller_protocol_;

  std::unique_ptr<crypto::Prover>            certificate_;
  generics::SharedWithLock<MainChainDetails> my_details_;
};

}  // namespace chain
}  // namespace fetch
