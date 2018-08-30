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

#include "constellation.hpp"
#include "http/middleware/allow_origin.hpp"
#include "ledger/chaincode/wallet_http_interface.hpp"
#include "network/p2pservice/explore_http_interface.hpp"
#include "network/p2pservice/p2p_http_interface.hpp"


namespace fetch {
  Constellation::~Constellation()
  {
    FETCH_LOG_INFO(LOGGING_NAME,"~Constellation");
  }
Constellation::Constellation(certificate_type &&certificate, uint16_t port_start,
                             std::size_t num_executors, std::size_t num_lanes,
                             std::size_t num_slices, std::string const &interface_address,
                             std::string const &db_prefix)
  : interface_address_{interface_address}
  , num_lanes_{static_cast<uint32_t>(num_lanes)}
  , num_slices_{static_cast<uint32_t>(num_slices)}
  , p2p_port_{static_cast<uint16_t>(port_start + P2P_PORT_OFFSET)}
  , http_port_{static_cast<uint16_t>(port_start + HTTP_PORT_OFFSET)}
  , lane_port_start_{static_cast<uint16_t>(port_start + STORAGE_PORT_OFFSET)}
  , main_chain_port_{static_cast<uint16_t>(port_start + MAIN_CHAIN_PORT_OFFSET)}
{
  FETCH_LOG_INFO(LOGGING_NAME,"Constellation :: ", interface_address, " P ", port_start, " E ",
                     num_executors, " S ", num_lanes, "x", num_slices);

  // determine how many threads the network manager will require
  std::size_t const num_network_threads =
      num_lanes * 2 + 10;  // 2 := Lane/Storage Server, Lane/Storage Client 10
                           // := provision for http and p2p

  // create the network manager
  network_manager_ = std::make_unique<fetch::network::NetworkManager>(num_network_threads);
  network_manager_->Start();  // needs to be started

  // Creating P2P instance
  p2p_ = std::make_unique<p2p::P2PService>(std::move(certificate), p2p_port_, *network_manager_);

  auto profile = p2p_->Profile();
  auto my_name = std::string(byte_array::ToBase64(profile.identity.identifier()));



  // setup the storage service
  storage_service_.Setup(db_prefix, num_lanes, lane_port_start_, *network_manager_, false);

  // create the aggregate storage client
  storage_ = std::make_shared<ledger::StorageUnitClient>(*network_manager_);
  for (std::size_t i = 0; i < num_lanes; ++i)
  {
    // We connect to the lanes
    crypto::Identity ident = storage_->AddLaneConnection<connection_type>(
        interface_address, static_cast<uint16_t>(lane_port_start_ + i));

    // ... and make the lane details available for the P2P module
    // to promote
    p2p_->AddLane(static_cast<uint32_t>(i), interface_address,
                  static_cast<uint16_t>(lane_port_start_ + i), ident);
  }

  // create the execution manager (and its executors)
  execution_manager_ =
      std::make_shared<ledger::ExecutionManager>(num_executors, storage_, [this]() {
        auto executor = CreateExecutor();
        executors_.push_back(executor);
        return executor;
      });

  execution_manager_->Start();

  // Main chain
  main_chain_service_ = std::make_unique<chain::MainChainService>(db_prefix, main_chain_port_,
                                                                  *network_manager_.get(), my_name);

  main_chain_service_->SetOwnerIdentity(profile.identity);

  // Mainchain remote
  main_chain_remote_ = std::make_unique<chain::MainChainRemoteControl>();
  client_type client(*network_manager_.get());
  client.Connect(interface_address, main_chain_port_);
  shared_service_type service = std::make_shared<service_type>(client, *network_manager_.get());
  main_chain_remote_->SetClient(service);

  // Mining and block coordination
  block_coordinator_  = std::make_unique<chain::BlockCoordinator>(*main_chain_service_->mainchain(),
                                                                 *execution_manager_);
  transaction_packer_ = std::make_unique<miner::AnnealerMiner>();
  main_chain_miner_   = std::make_unique<chain::MainChainMiner>(
      num_lanes_, num_slices_, *main_chain_service_->mainchain(), *block_coordinator_,
      *transaction_packer_, main_chain_port_);

  main_chain_miner_->onBlockComplete([this](const chain::MainChain::BlockType blk) {
    this->main_chain_service_->PublishBlock(blk);
  });

  tx_processor_ = std::make_unique<ledger::TransactionProcessor>(*storage_, *transaction_packer_);

  // Now that the execution manager is created, can start components that need
  // it to exist
  block_coordinator_->start();
  main_chain_miner_->start();

  // define the list of HTTP modules to be used
  http_modules_ = {
      std::make_shared<p2p::P2PHttpInterface>(main_chain_service_->mainchain(),
                                              main_chain_service_.get(),
                                              main_chain_service_->mainchainprotocol()),
      std::make_shared<ledger::ContractHttpInterface>(*storage_, *tx_processor_),
      std::make_shared<ledger::WalletHttpInterface>(*storage_, *tx_processor_),
      std::make_shared<p2p::ExploreHttpInterface>(p2p_.get(), main_chain_service_->mainchain())};

  // create and register the HTTP modules
  http_ = std::make_unique<http::HTTPServer>(http_port_, *network_manager_);
  http_->AddMiddleware(http::middleware::AllowOrigin("*"));
  for (auto const &module : http_modules_)
  {
    http_->AddModule(*module);
  }
}

void Constellation::Run(peer_list_type const &initial_peers)
{
  try
  {
    p2p_->AddMainChain(interface_address_, static_cast<uint16_t>(main_chain_port_));

    // Adding handle for the orchestration
    p2p_->OnPeerUpdateProfile([this](p2p::EntryPoint const &ep) {
        // std::cout << "MAKING CALL ::: " << std::endl;

        FETCH_LOG_INFO(LOGGING_NAME,"OnPeerUpdateProfile: ", byte_array::ToBase64(ep.identity.identifier()),
                       " mainchain?: ", ep.is_mainchain.load(), " lane:? ", ep.is_lane.load());

        if (ep.is_mainchain)
        {
          if (main_chain_remote_)
          {
            main_chain_remote_->TryConnect(ep);
          }
          else
          {
            FETCH_LOG_WARN(LOGGING_NAME,"Main chain remote is invalid, unable to dispatch this request");
          }
        }
        if (ep.is_lane)
        {
          FETCH_LOG_INFO(LOGGING_NAME,"Trying to make that connection noow.....");

          if (storage_)
          {
            storage_->TryConnect(ep);
          }
          else
          {
            FETCH_LOG_WARN(LOGGING_NAME,"Storage is not currently available");
          }
        }
      });

    p2p_->Start();

    // Make the initial p2p connections
    // Note that we first connect after setting up the lanes to prevent that nodes
    // will be too fast in trying to set up lane connections.
    for (auto const &peer : initial_peers)
    {
      FETCH_LOG_WARN(LOGGING_NAME,"Connecting to ", peer.address(), ":", peer.port());

      LOG_STACK_TRACE_POINT;

      p2p_->Connect(peer.address(), peer.port());
    }

    // monitor loop
    while (active_)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Still alive...");
      std::this_thread::sleep_for(std::chrono::seconds{5});
    }
  }
  catch (...) {
    FETCH_LOG_ERROR(LOGGING_NAME,"Exception locking mutex: Const::Run");
    throw;
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Exiting...");
}

}  // namespace fetch
