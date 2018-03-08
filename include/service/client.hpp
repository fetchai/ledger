#ifndef SERVICE_SERVICE_CLIENT_HPP
#define SERVICE_SERVICE_CLIENT_HPP

#include "service/callable_class_member.hpp"
#include "service/message_types.hpp"
#include "service/protocol.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

#include "service/error_codes.hpp"
#include "service/promise.hpp"
#include "service/can_subscribe.hpp"

#include "assert.hpp"
#include "network/tcp_client.hpp"
#include "mutex.hpp"
#include "logger.hpp"

#include <map>

namespace fetch 
{
namespace service 
{

template< typename T >  
class ServiceClient : public T, public CanSubscribe
{
public:
  typedef T super_type;

  typedef typename super_type::thread_manager_type thread_manager_type ;  
  typedef typename super_type::thread_manager_ptr_type thread_manager_ptr_type ;
  typedef typename thread_manager_type::event_handle_type event_handle_type;
   
  ServiceClient(std::string const& host,
    uint16_t const& port,
    thread_manager_ptr_type thread_manager) :
    super_type(host, port, thread_manager),
    thread_manager_(thread_manager),
    message_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;    
    running_ = true;

    // TODO: Replace with thread manager
    worker_thread_ = new std::thread([this]() 
      {
        this->ProcessMessages();
      });
  }

  ~ServiceClient() 
  {
    LOG_STACK_TRACE_POINT;    
    // TODO: Move to OnStop
    running_ = false;
    worker_thread_->join();
    delete worker_thread_; 
       
  }

  void PushMessage(network::message_type const& msg) override 
  {
    LOG_STACK_TRACE_POINT;
    
    std::lock_guard< fetch::mutex::Mutex > lock(message_mutex_);
    messages_.push_back(msg);
  }
  
  void ConnectionFailed() override
  {
    LOG_STACK_TRACE_POINT;

    this->ClearPromises();
  }
protected:
  virtual void DeliverRequest(network::message_type const&msg) override {
    super_type::Send(msg);
  }

private:
  

  void ProcessMessages() 
  {
    LOG_STACK_TRACE_POINT;
    
    while(running_) 
    {
      
      message_mutex_.lock();
      bool has_messages = (!messages_.empty());
      message_mutex_.unlock();
      
      while(has_messages) 
      {
        message_mutex_.lock();

        network::message_type msg;
        has_messages = (!messages_.empty());
        if(has_messages) 
        {
          msg = messages_.front();
          messages_.pop_front();
        };
        message_mutex_.unlock();
        
        if(has_messages) 
        {
          ProcessServerMessage( msg );
        }
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    }
  }
  


  thread_manager_ptr_type thread_manager_;



  std::atomic< bool > running_;
  std::deque< network::message_type > messages_;
  fetch::mutex::Mutex message_mutex_;  
  std::thread *worker_thread_ = nullptr;  
};
};
};

#endif
