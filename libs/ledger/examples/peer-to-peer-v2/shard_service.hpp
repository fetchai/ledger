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

#include "network/protocols.hpp"
#include "network/service/server.hpp"
#include "network/service/service_client.hpp"
#include "network/tcp/tcp_server.hpp"

#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"
#include "http/server.hpp"

#include "core/byte_array/encoders.hpp"
#include "crypto/hash.hpp"
#include "testing/unittest.hpp"

#include "http/middleware/allow_origin.hpp"
#include "http/middleware/color_log.hpp"
#include "http/server.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

class FetchChainKeeperService : public fetch::protocols::ChainKeeperProtocol
{
public:
  FetchChainKeeperService(uint16_t const &port, uint16_t const &http_port,
                          fetch::network::NetworkManager *tm)
    : fetch::protocols::ChainKeeperProtocol(tm, fetch::protocols::FetchProtocols::CHAIN_KEEPER,
                                            details_)
    , network_manager_(tm)
    , service_(port, network_manager_)
    , http_server_(http_port, network_manager_)
  {
    LOG_STACK_TRACE_POINT;
    using namespace fetch::protocols;
    std::cout << "ChainKeeper listening for peers on " << (port) << ", clients on " << (http_port)
              << std::endl;

    details_.port      = port;
    details_.http_port = http_port;

    // Creating a service contiaing the group protocol
    service_.Add(FetchProtocols::CHAIN_KEEPER, this);
    running_ = false;

    start_event_ = network_manager_->OnAfterStart([this]() {
      running_ = true;
      network_manager_->Post([this]() { this->SyncTransactions(); });
    });

    stop_event_ = network_manager_->OnBeforeStop([this]() { running_ = false; });

    http_server_.AddMiddleware(fetch::http::middleware::AllowOrigin("*"));
    http_server_.AddMiddleware(fetch::http::middleware::ColorLog);
    http_server_.AddModule(*this);

    using namespace fetch::http;
    http_server_.AddView(Method::GET, "/mining-power/(power=\\d+)",
                         [this](ViewParameters const &params, HTTPRequest const &req) {
                           HTTPResponse res("{}");
                           this->difficulty_mutex_.lock();
                           this->difficulty_ = params["power"].AsInt();
                           fetch::logger.Highlight("Mine power set to: ", this->difficulty_);

                           this->difficulty_mutex_.unlock();
                           return res;
                         });
  }

  ~FetchChainKeeperService()
  {
    network_manager_->Off(start_event_);
    network_manager_->Off(stop_event_);
  }

  /*
   * State maintenance
   * ═══════════════════════════════════════════
   * The group nodes continuously pull data from
   * its peers. Each node is responsible for
   * requesting the data they want themselves.
   *
   * ┌─────────────────────────────────────────┐
   * │            Sync Transactions            │◀─┐
   * └────────────────────┬────────────────────┘  │
   *                      │                       │
   * ┌────────────────────▼────────────────────┐  │
   * │               Sync Blocks               │  │
   * └────────────────────┬────────────────────┘  │
   *                      │                       │
   * ┌────────────────────▼────────────────────┐  │
   * │                  Mine                   │──┘
   * └─────────────────────────────────────────┘
   */

  std::vector<transaction_type> txs_;
  void                          SyncTransactions()
  {

    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    using namespace fetch::protocols;

    std::unordered_map<tx_digest_type, transaction_type, fetch::crypto::CallableFNV>
        incoming_transactions;
    // Get missing transactions

    // Get applied transactions (after N)

    // Get latest transactions
    std::vector<fetch::service::Promise> promises;

    this->with_peers_do([&promises](std::vector<client_shared_ptr_type> const &clients) {
      for (auto const &c : clients)
      {
        promises.push_back(c->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::GET_TRANSACTIONS));
      }
    });

    txs_.reserve(1000);

    for (auto &p : promises)
    {
      txs_.clear();
      p.As<std::vector<transaction_type>>(txs_);

      for (auto &tx : txs_)
      {
        tx.UpdateDigest();
        incoming_transactions[tx.digest()] = tx;
      }
    }

    this->AddBulkTransactions(incoming_transactions);

    // Get unapplied transactions
    if (running_)
    {
      network_manager_->Post([this]() { this->SyncChain(); });
    }
  }

  void SyncChain()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    using namespace fetch::protocols;

    ///////////
    // All blocks
    /*
    std::vector< block_type > blocks;
    std::vector< fetch::service::Promise > promises;
    this->with_peers_do([&promises](std::vector< client_shared_ptr_type >
clients, std::vector< EntryPoint > const&) { for(auto &c: clients) {
          promises.push_back( c->Call(FetchProtocols::CHAIN_KEEPER,
ChainKeeperRPC::GET_BLOCKS ) );
        }
      });

    std::vector< block_type > newblocks;
    newblocks.reserve(1000);

    for(auto &p: promises) {
      p.As( newblocks );
      this->AddBulkBlocks( newblocks );
    }



    for(std::size_t i=0; i < 100; ++i) std::cout << "=";
    std::cout << std::endl;

    std::cout << "Chain stats:" << std::endl;
    std::cout << "Synced blocks: " << newblocks.size() << std::endl;
    std::cout << "Block count: " << this->block_count() << std::endl;
    std::cout << "Transaction count: " << this->transaction_count() <<
std::endl; std::cout << "Unapplied transaction count: " <<
this->unapplied_transaction_count() << std::endl; std::cout << "Applied
transaction count: " << this->applied_transaction_count() << std::endl;
    for(std::size_t i=0; i < 100; ++i) std::cout << "=";
    std::cout << std::endl;
//    std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );

    if( (this->unapplied_transaction_count() == 0) &&
      (this->applied_transaction_count() > 0 ) ) {

      for(std::size_t i=0; i < 100; ++i)
        std::cout << "ALL SYNCED " << this->applied_transaction_count() << " ";

      //      std::this_thread::sleep_for( std::chrono::milliseconds(5000) );

    }

    */

    if (running_)
    {
      network_manager_->Post([this]() { this->Mine(); });
    }
  }

  void Mine()
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    /*
    difficulty_mutex_.lock();
    int diff = difficulty_;
    difficulty_mutex_.unlock();
    if(diff == 0) {
      fetch::logger.Debug("Exiting mining because diff = 0");
      if(running_) {
        network_manager_->Post([this]() {
            this->SyncTransactions();
          });
      }
      return;
    }



    for(std::size_t i = 0; i < 100; ++i) {
      fetch::logger.Highlight("Mining cycle ", i);

      auto block = this->GetNextBlock();
      if(  block.body().transaction_hashes.size() == 0) {
        fetch::logger.Highlight("--------======= NO TRRANSACTIONS TO MINE
=========--------");
//        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ));
        break;
      } else {
//        std::chrono::system_clock::time_point started =
std::chrono::system_clock::now(); std::cout << "Mining at difficulty " << diff
<< std::endl; auto &p = block.proof();

        p.SetTarget( diff );
        ++p;
        p();
        double work = fetch::math::Log( p.digest() ); // TODO: Check formula
        block.set_weight(work);
        block.set_total_weight(block.total_weight() +work);
//        while(!p()) ++p;



//        std::chrono::system_clock::time_point end =
std::chrono::system_clock::now();
//        double ms =  std::chrono::duration_cast<std::chrono::milliseconds>(end
- started).count();
//        TODO("change mining mechanism: ", ms);

//  if( ms < 500 ) {
//  std::this_thread::sleep_for( std::chrono::milliseconds( int( (500. - ms)  )
) );
//  }


        this->PushBlock( block );
      }

    }
    */

    if (running_)
    {
      network_manager_->Post([this]() { this->SyncTransactions(); });
    }
  }

  uint16_t port() const { return details_.port; }

private:
  int                         difficulty_ = 1;
  mutable fetch::mutex::Mutex difficulty_mutex_;

  fetch::network::NetworkManager *                         network_manager_;
  fetch::service::ServiceServer<fetch::network::TCPServer> service_;
  fetch::http::HTTPServer                                  http_server_;
  fetch::protocols::EntryPoint                             details_;

  typename fetch::network::NetworkManager::event_handle_type start_event_;
  typename fetch::network::NetworkManager::event_handle_type stop_event_;
  std::atomic<bool>                                          running_;

  //  fetch::http::HTTPServer http_server_;
};
