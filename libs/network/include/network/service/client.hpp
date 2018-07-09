#ifndef SERVICE_SERVICE_CLIENT_HPP
#define SERVICE_SERVICE_CLIENT_HPP

#include "core/serializers/referenced_byte_array.hpp"
#include "core/serializers/serializable_exception.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/message_types.hpp"
#include "network/service/protocol.hpp"

#include "network/service/client_interface.hpp"
#include "network/service/error_codes.hpp"
#include "network/service/promise.hpp"
#include "network/service/server_interface.hpp"

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/tcp/tcp_client.hpp"

#include <map>

namespace fetch {
namespace service {

//template <typename T>
class ServiceClient : public ServiceClientInterface,
                      public ServiceServerInterface {
 public:


  //typedef T super_type;
  typedef network::ThreadManager thread_manager_type;

  ServiceClient(std::shared_ptr< network::AbstractConnection > connection,
    thread_manager_type thread_manager)
    : connection_(connection),
      thread_manager_(thread_manager),
      message_mutex_(__LINE__, __FILE__) 
  {
      
    connection_->OnMessage([this](network::message_type const& msg) {
        LOG_STACK_TRACE_POINT;
        
        {
          std::lock_guard<fetch::mutex::Mutex> lock(message_mutex_);
          messages_.push_back(msg);
        }
        
        // Since this class isn't shared_from_this, try to ensure safety when destructing
        thread_manager_.Post([this]()
          {
            ProcessMessages();
          });
      });
    
      /*
      ptr->OnConnectionFailed([this]() {
          // TODO: Clear closures?
        });
      */
  }

  ServiceClient(network::TCPClient &connection,
    thread_manager_type thread_manager)
    : ServiceClient(connection.connection_pointer().lock(), thread_manager)
  { }
  

  ~ServiceClient()
  {
    LOG_STACK_TRACE_POINT;

    // Disconnect callbacks
    if(!connection_->Closed()) {
      connection_->ClearClosures();
      connection_->Close();
      
      int timeout = 100;
      
      // Can only guarantee we are not being called when socket is closed
      while(!connection_->Closed())
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timeout--;
        
        if(timeout == 0) break;
      }
    }
    
  }

  void Close() 
  {
    connection_->Close();
    connection_.reset();    
  }    

  connection_handle_type handle() const 
  {
    return connection_->handle();
  }
  
  bool is_alive() const 
  {
    return connection_->is_alive();
  }
  
 protected:
  bool DeliverRequest(network::message_type const& msg) override {
    if(connection_->Closed()) return false;
    
    connection_->Send(msg);
    return true;

  }

  bool DeliverResponse(connection_handle_type, network::message_type const& msg) override {
    if(connection_->Closed()) return false;
    
    connection_->Send(msg);
    return true;

  }

 private:
  std::shared_ptr< network::AbstractConnection > connection_;  
  void ProcessMessages() {
    LOG_STACK_TRACE_POINT;

    message_mutex_.lock();
    bool has_messages = (!messages_.empty());
    message_mutex_.unlock();

    while (has_messages) {
      message_mutex_.lock();

      network::message_type msg;
      has_messages = (!messages_.empty());
      if (has_messages) {
        msg = messages_.front();
        messages_.pop_front();
      };
      message_mutex_.unlock();

      if (has_messages) {
        // TODO: Post
        if (!ProcessServerMessage(msg)) {
          fetch::logger.Debug("Looking for RPC functionality");

          if (!PushProtocolRequest(connection_handle_type(-1), msg)) {
            throw serializers::SerializableException(
                error::UNKNOWN_MESSAGE,
                byte_array::ConstByteArray("Unknown message"));
          }
        }
      }
    }
  }

  thread_manager_type               thread_manager_;
  std::deque<network::message_type> messages_;
  mutable fetch::mutex::Mutex       message_mutex_;

};
}
}

#endif
