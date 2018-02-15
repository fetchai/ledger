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
            this->Mine();            
          });
        thread_manager_->io_service().post([this]() {
            this->SyncChain(); 
          });        
      });

    stop_event_ = thread_manager_->OnBeforeStop([this]() {
        running_ = false;
      });

    http_server_.AddMiddleware( fetch::http::middleware::AllowOrigin("*") );       
    http_server_.AddMiddleware( fetch::http::middleware::ColorLog);
    http_server_.AddModule(*this);
  }
  
  ~FetchShardService() 
  {
    thread_manager_->Off( start_event_ );
    thread_manager_->Off( stop_event_ );
  }

  void Mine() 
  {
    int diff = 14;    

    auto block = this->GetNextBlock();
    if(  block.body().transaction_hash == "") {
      std::this_thread::sleep_for( std::chrono::milliseconds( 100 ));           
    } else {
      std::cout << "Mining at difficulty " << diff << std::endl;    
      auto &p = block.proof();
      
      p.SetTarget( diff );
      while(!p()) ++p;

      this->PushBlock( block );
    }
    
    if(running_) {
      thread_manager_->io_service().post([this]() {
          this->Mine();            
        });    
    }
  }

  void SyncChain() 
  {
    using namespace fetch::protocols;
    
    std::vector< block_header_type > headers;


    this->with_loose_chains_do([&headers]( std::map< uint64_t, ShardManager::PartialChain > const &chains ) {
        for(auto const &c: chains)
        {
          headers.push_back(c.second.next_missing);          
        }        
      });


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
    

    
    if(running_) {
      thread_manager_->io_service().post([this]() {
          this->SyncChain();          
        });    
    }    
  }
  
  
  uint16_t port() const 
  {
    return details_.port;    
  }

  
private:

  
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
