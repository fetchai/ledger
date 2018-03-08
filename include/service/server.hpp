#ifndef SERVICE_SERVER_HPP
#define SERVICE_SERVER_HPP

#include "service/callable_class_member.hpp"
#include "service/message_types.hpp"
#include "service/protocol.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

#include "service/error_codes.hpp"
#include "service/promise.hpp"
#include "service/client_interface.hpp"
#include "service/server_interface.hpp"
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
class ServiceServer : public T, public ServiceServerInterface
{
public:
  typedef T super_type;
  typedef ServiceServer< T > self_type;
  
  typedef typename super_type::thread_manager_type thread_manager_type ;  
  typedef typename super_type::thread_manager_ptr_type thread_manager_ptr_type ;
  typedef typename thread_manager_type::event_handle_type event_handle_type;  
  typedef typename T::handle_type handle_type;

  // TODO Rename
  class ClientRPCInterface : public ServiceClientInterface 
  {
  public:
    ClientRPCInterface() = delete;
    
    ClientRPCInterface(ClientRPCInterface const&) = delete;
    ClientRPCInterface const& operator=(ClientRPCInterface const&) = delete;
    
    ClientRPCInterface( self_type *server, handle_type client):
      server_(server),
      client_(client)   
    { }

    bool ProcessMessage(network::message_type const& msg) 
    {
      return this->ProcessServerMessage(msg);      
    }
    
  protected:
    void DeliverRequest(network::message_type const&msg) override
    {
      server_->Send(client_, msg);      
    }
  private:
    self_type *server_; // TODO: Change to shared ptr and add enable_shared_from_this on service
    handle_type client_;    
  };
  
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

    client_rpcs_mutex_.lock();
    
    for(auto &c: client_rpcs_)
    {
      delete c.second;      
    }

    client_rpcs_mutex_.unlock();    
    
  }


  ClientRPCInterface& ServiceInterfaceOf(handle_type const&i)
  {
    std::lock_guard< fetch::mutex::Mutex > lock(client_rpcs_mutex_);

    if(client_rpcs_.find(i) == client_rpcs_.end())
    {
      // TODO: Make sure to delete this on disconnect after all promises has been fulfilled
      client_rpcs_.emplace( std::make_pair(i, new ClientRPCInterface(this, i) ));
    }

    return *client_rpcs_[i];
  }
  
  

protected:
  bool DeliverResponse(handle_type client, network::message_type const& msg) override {
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
            if(!this->PushProtocolRequest( pm.client, pm.message ))
            {
              bool processed = false;

              client_rpcs_mutex_.lock(); 
              if(client_rpcs_.find(pm.client) !=client_rpcs_.end())
              {
                auto &c = client_rpcs_[pm.client];
                processed = c->ProcessMessage(pm.message);                
              }
              client_rpcs_mutex_.unlock();
              
              if(!processed)
              {                
                // TODO: Lookup client RPC handler
                fetch::logger.Error("Possibly a response to a client?");
                
                throw serializers::SerializableException(
                  error::UNKNOWN_MESSAGE, "Unknown message");
                TODO_FAIL( "call type not implemented yet");
              }
              
            }
            
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
  mutable fetch::mutex::Mutex message_mutex_;
  std::atomic< bool > running_;


  mutable fetch::mutex::Mutex client_rpcs_mutex_;
  std::map< handle_type, ClientRPCInterface*  > client_rpcs_;    

};
};
};

#endif
