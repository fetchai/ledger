#ifndef SHARD_SERVICE_HPP
#define SHARD_SERVICE_HPP

#include "service/client.hpp"
#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols.hpp"


#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"

#include"crypto/hash.hpp"
#include"unittest.hpp"
#include"byte_array/encoders.hpp"

#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"


#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <algorithm>

class FetchShardService : public fetch::protocols::ShardProtocol {
public:
  FetchShardService(uint16_t const &port, uint16_t const& http_port, fetch::network::ThreadManager *tm ) :    
    fetch::protocols::ShardProtocol(tm, fetch::protocols::FetchProtocols::SHARD,  details_),
    thread_manager_( tm),
    service_(port, thread_manager_),
    http_server_(http_port, thread_manager_)    
  {
    using namespace fetch::protocols;    
    std::cout << "Shard listening for peers on " << (port) << ", clients on " << ( http_port ) << std::endl;

    details_.port = port;
    details_.http_port = http_port;    
          
    // Creating a service contiaing the shard protocol
    service_.Add(FetchProtocols::SHARD, this);
    running_ = false;
    
    start_event_ = thread_manager_->OnAfterStart([this]() {
        running_ = true;        
        thread_manager_->io_service().post([this]() {
            this->SyncTransactions(); 
          });        
      });

    stop_event_ = thread_manager_->OnBeforeStop([this]() {
        running_ = false;
      });

    http_server_.AddMiddleware( fetch::http::middleware::AllowOrigin("*") );       
    http_server_.AddMiddleware( fetch::http::middleware::ColorLog);
    http_server_.AddModule(*this);

    using namespace fetch::http;
    http_server_.AddView(Method::GET, "/mining-power/(power=\\d+)", [this](ViewParameters const &params, HTTPRequest const &req) {
      HTTPResponse res("{}");
      this->difficulty_mutex_.lock();      
      this->difficulty_ = params["power"].AsInt();
      fetch::logger.Highlight("Mine power set to: ", this->difficulty_ );
      
      this->difficulty_mutex_.unlock();            
      return res;      
      } );
    
  }
  
  ~FetchShardService() 
  {
    thread_manager_->Off( start_event_ );
    thread_manager_->Off( stop_event_ );
  }


/*                                               
 * State maintenance                             
 * ═══════════════════════════════════════════   
 * The shard nodes continuously pull data from   
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

  std::vector< transaction_type > txs_;
  void SyncTransactions() 
  {

    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    using namespace fetch::protocols;

    std::unordered_map< tx_digest_type, transaction_type, fetch::crypto::CallableFNV > incoming_transactions;
    // Get missing transactions
    
    // Get applied transactions (after N)

    // Get latest transactions
    std::vector< fetch::service::Promise > promises;
    
    this->with_peers_do([this, &promises](  std::vector< client_shared_ptr_type > const &clients ) {
        for(auto const &c: clients) {
          promises.push_back( c->Call(FetchProtocols::SHARD , ShardRPC::GET_TRANSACTIONS) );
        }
      });


    txs_.reserve(1000);

    
    for(auto &p: promises) {
      txs_.clear();
      p.As< std::vector< transaction_type > >( txs_ );      

      for(auto &tx: txs_) {
        tx.UpdateDigest();
        incoming_transactions[ tx.digest() ] = tx;        
      }
    }
    
    
    this->AddBulkTransactions( incoming_transactions );
    
    // Get unapplied transactions
    if(running_) {
      thread_manager_->Post([this]() {
          this->SyncChain();          
        });    
    }
  }
  

  void SyncChain() 
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    using namespace fetch::protocols;

    //////////
    // Missing blocks
    
    // Working out which blocks are missing
    std::vector< block_header_type > headers;
    this->with_loose_chains_do([&headers]( std::map< uint64_t, ChainManager::PartialChain > const &chains ) {
        for(auto const &c: chains)
        {
          headers.push_back(c.second.next_missing);          
        }        
      });


    // Fetching them
    if(headers.size() != 0) {
      std::vector< block_type > blocks;
      
      this->with_peers_do([&headers, &blocks](std::vector< client_shared_ptr_type > clients, std::vector< EntryPoint > const&)
        {
          std::random_shuffle( clients.begin(), clients.end() );
          for(auto &h: headers)
          {
            for(auto &c: clients)
            {
              std::vector< block_type > nb = c->Call(FetchProtocols::SHARD, ShardRPC::REQUEST_BLOCKS_FROM, h, uint16_t(10) ).As< std::vector< block_type > >();
              if(nb.size() != 0)
              {
                for(auto &b: nb)
                {
                  blocks.push_back(b);                  
                }
                
                break;                
              }
            }
          }
          
        });

      for(auto &b: blocks)
      {
        this->PushBlock(b);        
      }
    }

    ///////////
    // All blocks
    /*
    std::vector< block_type > blocks;
    std::vector< fetch::service::Promise > promises;    
    this->with_peers_do([&promises](std::vector< client_shared_ptr_type > clients, std::vector< EntryPoint > const&) {
        for(auto &c: clients) {
          promises.push_back( c->Call(FetchProtocols::SHARD, ShardRPC::GET_BLOCKS ) );
        }
      });

    std::vector< block_type > newblocks;
    newblocks.reserve(1000);
    
    for(auto &p: promises) {
      p.As( newblocks );
      fetch::logger
      this->AddBulkBlocks( newblocks );
    }
    
    */    
    
    for(std::size_t i=0; i < 100; ++i) std::cout << "=";
    std::cout << std::endl;
    
    std::cout << "Chain stats:" << std::endl;
    std::cout << "Block count: " << this->block_count() << std::endl;    
    std::cout << "Transaction count: " << this->transaction_count() << std::endl;
    std::cout << "Unapplied transaction count: " << this->unapplied_transaction_count() << std::endl;
    std::cout << "Applied transaction count: " << this->applied_transaction_count() << std::endl;        
    for(std::size_t i=0; i < 100; ++i) std::cout << "=";
    std::cout << std::endl;
    if( (this->unapplied_transaction_count() == 0) &&
      (this->applied_transaction_count() > 0 ) ) {
      
      for(std::size_t i=0; i < 100; ++i)
        std::cout << "ALL SYNCED " << this->applied_transaction_count() << " ";
      
      std::this_thread::sleep_for( std::chrono::milliseconds(5000) );
      
    }
    

    
    if(running_) {
      thread_manager_->Post([this]() {
          this->Mine();
        });
    }    
  }

  void Mine() 
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    for(std::size_t i = 0; i < 1000; ++i) {
      
      difficulty_mutex_.lock();
      int diff = difficulty_;
      difficulty_mutex_.unlock();
    
      if(diff == 0) {
        fetch::logger.Debug("Exiting mining because diff = 0");            
        if(running_) {
          thread_manager_->Post([this]() {
              this->SyncTransactions(); 
            }); 
        } 
        return;
      }
    
    
      auto block = this->GetNextBlock();
      if(  block.body().transaction_hash == "") {
        fetch::logger.Highlight("--------======= NO TRRANSACTIONS TO MINE =========--------");
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ));
        break;        
      } else {
//        std::chrono::system_clock::time_point started =  std::chrono::system_clock::now();              
        std::cout << "Mining at difficulty " << diff << std::endl;    
        auto &p = block.proof();
      
        p.SetTarget( diff );
        ++p;
        
//        while(!p()) ++p;

//        std::chrono::system_clock::time_point end =  std::chrono::system_clock::now();
//        double ms =  std::chrono::duration_cast<std::chrono::milliseconds>(end - started).count();
//        TODO("change mining mechanism: ", ms);
/*
  if( ms < 500 ) {
  std::this_thread::sleep_for( std::chrono::milliseconds( int( (500. - ms)  ) ) ); 
  }
*/    
      
        this->PushBlock( block );
      }

    }
    
    
    if(running_) {
      thread_manager_->Post([this]() {
          this->SyncTransactions(); 
        }); 
    }    
  }

  
  uint16_t port() const 
  {
    return details_.port;    
  }

  
private:
  int difficulty_ = 0;
  mutable fetch::mutex::Mutex difficulty_mutex_;
  
  fetch::network::ThreadManager *thread_manager_;    
  fetch::service::ServiceServer< fetch::network::TCPServer > service_;
  fetch::http::HTTPServer http_server_;
  fetch::protocols::EntryPoint details_;
  
  typename fetch::network::ThreadManager::event_handle_type start_event_;
  typename fetch::network::ThreadManager::event_handle_type stop_event_;  
  std::atomic< bool > running_;
  
//  fetch::http::HTTPServer http_server_;   
};

#endif 
