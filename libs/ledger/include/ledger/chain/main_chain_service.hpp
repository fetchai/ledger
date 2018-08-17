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

#include "network/service/server.hpp"
#include "storage/object_store.hpp"
#include "storage/object_store_protocol.hpp"
#include "storage/object_store_syncronisation_protocol.hpp"

#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/main_chain_controller.hpp"
#include "ledger/chain/main_chain_controller_protocol.hpp"
#include "ledger/chain/main_chain_details.hpp"
#include "ledger/chain/main_chain_identity.hpp"
#include "ledger/chain/main_chain_identity_protocol.hpp"
#include "ledger/chain/main_chain_protocol.hpp"
#include "network/management/connection_register.hpp"

namespace fetch {
namespace chain {

class MainChainService : public service::ServiceServer<fetch::network::TCPServer>
{
public:
  using proof_type = fetch::chain::MainChain::proof_type;
  using block_type = fetch::chain::MainChain::block_type;
  using body_type  = fetch::chain::MainChain::block_type::body_type;
  using block_hash = fetch::chain::MainChain::block_hash;

  using connectivity_details_type = MainChainDetails;
  using client_register_type      = fetch::network::ConnectionRegister<connectivity_details_type>;

  using mainchain_type          = fetch::chain::MainChain;
  using mainchain_protocol_type = fetch::chain::MainChainProtocol<client_register_type>;

  using block_store_type          = storage::ObjectStore<block_type>;
  using block_store_protocol_type = storage::ObjectStoreProtocol<block_type>;
  using block_sync_protocol_type =
      storage::ObjectStoreSyncronisationProtocol<client_register_type, block_type>;

  using controller_type          = MainChainController;
  using controller_protocol_type = MainChainControllerProtocol;

  using identity_type          = MainChainIdentity;
  using identity_protocol_type = MainChainIdentityProtocol;
  using connection_handle_type = client_register_type::connection_handle_type;
  using super_type             = service::ServiceServer<fetch::network::TCPServer>;

  using thread_pool_type = network::ThreadPool;

  enum
  {
    IDENTITY = 1,
    CHAIN,
    CONTROLLER
  };

  MainChainService(std::string const &db_dir, uint16_t port, fetch::network::NetworkManager tm,
                   bool start_sync = true)
    : super_type(port, tm)
  {

    fetch::logger.Warn("Establishing mainchain Service on rpc://127.0.0.1:", port);

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
    identity_          = std::make_shared<identity_type>(register_, tm);
    identity_protocol_ = std::make_unique<identity_protocol_type>(identity_.get());
    this->Add(IDENTITY, identity_protocol_.get());

    mainchain_ = std::make_unique<mainchain_type>();
    mainchain_protocol_ =
        std::make_unique<mainchain_protocol_type>(CHAIN, register_, thread_pool_, mainchain_.get());
    this->Add(CHAIN, mainchain_protocol_.get());

    controller_          = std::make_unique<controller_type>(IDENTITY, identity_, register_, tm);
    controller_protocol_ = std::make_unique<controller_protocol_type>(controller_.get());
    this->Add(CONTROLLER, controller_protocol_.get());

    thread_pool_->Start();

    mainchain_protocol_->Start();
  }

  ~MainChainService()
  {

    identity_protocol_.reset();
    identity_.reset();
  }

  mainchain_type *mainchain() { return mainchain_.get(); }

private:
  client_register_type register_;
  thread_pool_type     thread_pool_;

  std::shared_ptr<identity_type>          identity_;
  std::unique_ptr<identity_protocol_type> identity_protocol_;

  std::unique_ptr<mainchain_type>          mainchain_;
  std::unique_ptr<mainchain_protocol_type> mainchain_protocol_;

  std::unique_ptr<controller_type>          controller_;
  std::unique_ptr<controller_protocol_type> controller_protocol_;
};

}  // namespace chain
}  // namespace fetch
