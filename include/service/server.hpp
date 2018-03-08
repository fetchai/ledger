#ifndef SERVICE_SERVER_HPP
#define SERVICE_SERVER_HPP

#include "service/callable_class_member.hpp"
#include "service/message_types.hpp"
#include "service/protocol.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

#include "service/error_codes.hpp"
#include "service/promise.hpp"
#include "service/has_protocol.hpp"
#include "mutex.hpp"

#include "assert.hpp"
#include "network/tcp_server.hpp"
#include "logger.hpp"

#include <map>
#include <atomic>
#include <mutex>
#include <deque>

namespace fetch 
{
namespace service 
{

template< typename T >
class ServiceServer : private T, public HasProtocol
{
public:
  typedef T super_type;

  typedef typename super_type::thread_manager_type thread_manager_type ;  
  typedef typename super_type::thread_manager_ptr_type thread_manager_ptr_type ;
  typedef typename thread_manager_type::event_handle_type event_handle_type;
  
  typedef typename T::handle_type handle_type;
  struct PendingMessage 
  {
    handle_type client;
    network::message_type message;
  };
  typedef byte_array::ConstByteArray byte_array_type;

  ServiceServer(uint16_t port, thread_manager_ptr_type thread_manager) :
    super_type(port, thread_manager),
    thread_manager_( thread_manager),
    message_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;
    
    running_ = false;
    event_service_start_ = thread_manager->OnBeforeStart([this]()      
      {
        this->running_ = true;        
        this->thread_manager_->io_service().post([this]() {
            this->ProcessMessages();
          });        
      } );
    event_service_stop_ = thread_manager->OnBeforeStop([this]()      
      {
        this->running_ = false;        
      } );     
  }

  ~ServiceServer() 
  {
    LOG_STACK_TRACE_POINT;
    
    thread_manager_->Off( event_service_start_ );
    thread_manager_->Off( event_service_stop_ );    
  }

  
protected:
  bool DeliverMessage(handle_type client, network::message_type const& msg) override {
    return super_type::Send( client, msg );
  }
  

private:
  void PushRequest(handle_type client,
    network::message_type const& msg) override 
  {
    LOG_STACK_TRACE_POINT;
    
    std::lock_guard< fetch::mutex::Mutex > lock(message_mutex_);
    fetch::logger.Info("RPC call from ", client);    
    PendingMessage pm = {client, msg};    
    messages_.push_back(pm);
  }

  void ProcessMessages() 
  {
    LOG_STACK_TRACE_POINT;
    
    message_mutex_.lock();
    bool has_messages = (!messages_.empty());
    message_mutex_.unlock();
    
    while(has_messages) 
    {
      message_mutex_.lock();      
      
      PendingMessage pm;
      fetch::logger.Debug("Server side backlog: ", messages_.size());
      has_messages = (!messages_.empty());
      if(has_messages) { // To ensure we can make a worker pool in the future
        pm = messages_.front();
        messages_.pop_front();
      };
      
      
      message_mutex_.unlock();
      
      if(has_messages) 
      {
        thread_manager_->io_service().post([this, pm]() { 
            fetch::logger.Debug("Processing message call");
            this->PushProtocolMessage( pm.client, pm.message );
          });
      }
    }
    
    if(running_) {
      thread_manager_->io_service().post([this]() { this->ProcessMessages(); } );  
    }
    
  }


  thread_manager_ptr_type thread_manager_;
  event_handle_type event_service_start_;
  event_handle_type event_service_stop_;    
  
  std::deque< PendingMessage > messages_;
  fetch::mutex::Mutex message_mutex_;
  std::atomic< bool > running_;


};
};
};

#endif
