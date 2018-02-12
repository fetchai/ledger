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
    mining_ = false;
    
    start_event_ = thread_manager_->OnAfterStart([this]() {
        mining_ = true;        
        thread_manager_->io_service().post([this]() {
            this->Mine();            
          });
      });

    stop_event_ = thread_manager_->OnBeforeStop([this]() {
        mining_ = false;
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
      this->Commit();      
    }
    
    if(mining_) {
      thread_manager_->io_service().post([this]() {
          this->Mine();            
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
  std::atomic< bool > mining_;
  
//  fetch::http::HTTPServer http_server_;   
};

#endif 
