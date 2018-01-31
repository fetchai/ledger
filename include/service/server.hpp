#ifndef SERVICE_SERVER_HPP
#define SERVICE_SERVER_HPP

#include "service/callable_class_member.hpp"
#include "service/message_types.hpp"
#include "service/protocol.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

#include "service/error_codes.hpp"
#include "service/promise.hpp"
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
class ServiceServer : public T
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
    thread_manager_->Off( event_service_start_ );
    thread_manager_->Off( event_service_stop_ );    
  }
  
  void PushRequest(handle_type client,
    network::message_type const& msg) override 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(message_mutex_);
    fetch::logger.Info("RPC call from ", client);    
    PendingMessage pm = {client, msg};    
    messages_.push_back(pm);
  }

  void Add(protocol_handler_type const& name, Protocol* protocol) 
  {
    if(members_[name] != nullptr) 
    {
      throw serializers::SerializableException(
        error::PROTOCOL_EXISTS,
        byte_array_type("Member already exists: "));
    }

    members_[name] = protocol;

    for(auto &feed: protocol->feeds()) 
    {
      feed->AttachToService( this );
    }
  }

private:
  void Call(serializer_type& result, handle_type const &client, serializer_type params) 
  {
    protocol_handler_type protocol;
    function_handler_type function;
    params >> protocol >> function;

    if(members_[protocol] == nullptr) 
    {
      throw serializers::SerializableException(
        error::PROTOCOL_NOT_FOUND,
        byte_array_type("Could not find protocol: "));
    }

    auto& mod = *members_[protocol] ; //*it->second;
    auto& fnc = mod[function];


    // If we need to add client id to function arguments
    if(fnc.meta_data() & Callable::CLIENT_ID_ARG ) {
      serializer_type newparams;
      // TODO: A prettier solution can be made with template parameters
      newparams << client;
      newparams.Allocate(params.bytes_left());
      auto const carr = params.data().SubArray( params.Tell(),params.bytes_left());      
      newparams.WriteBytes( carr.pointer(), params.bytes_left() );
      newparams.Seek(0);      
      return fnc(result, newparams);
    } 
      
    
    
    return fnc(result, params);
  }

  void ProcessMessages() 
  {

    message_mutex_.lock();
    bool has_messages = (!messages_.empty());
    message_mutex_.unlock();
    
    while(has_messages) 
    {
      message_mutex_.lock();      
      
      PendingMessage pm;
      has_messages = (!messages_.empty());
      if(has_messages) { // To ensure we can make a worker pool in the future
        pm = messages_.front();
        messages_.pop_front();
      };
      message_mutex_.unlock();
      
      if(has_messages) 
      {
        ProcessClientMessage( pm.client, pm.message );
      }
    }
    


    if(running_) {
      thread_manager_->io_service().post([this]() { this->ProcessMessages(); } );  
    }
    
  }


  void ProcessClientMessage(handle_type client,
    network::message_type const& msg) 
  {
    serializer_type params(msg);
      
    service_classification_type type;
    params >> type;

    if (type == SERVICE_FUNCTION_CALL) 
    {
      serializer_type result;
      Promise::promise_counter_type id;
      
      try 
      {
        params >> id;
        result << SERVICE_RESULT << id;
        Call(result, client, params);
      } catch (serializers::SerializableException const& e) 
      {
        result = serializer_type();
        result << SERVICE_ERROR << id << e;
      }

      this->Send(client, result.data());
    } else if  (type == SERVICE_SUBSCRIBE)  
    {
      protocol_handler_type protocol;
      feed_handler_type feed;
      subscription_handler_type subid;
      
      try 
      {
        params >> protocol >> feed >> subid;
        auto& mod = *members_[protocol] ;      

        mod.Subscribe( client, feed, subid );
        
      } catch (serializers::SerializableException const& e) 
      {
        // FIX Serialization of errors such that this also works
        /*
          serializer_type result;
          result = serializer_type();
          result << SERVICE_ERROR << id << e;
          Send(client, result.data());
        */
        throw e;
      }
      
    } else if  (type == SERVICE_UNSUBSCRIBE)  
    {
      protocol_handler_type protocol;
      feed_handler_type feed;
      subscription_handler_type subid;
      
      try 
      {
        params >> protocol >> feed >> subid;
        auto& mod = *members_[protocol] ;      

        mod.Unsubscribe( client, feed, subid );
        
      } catch (serializers::SerializableException const& e) 
      {
        // FIX Serialization of errors such that this also works
        /*
          serializer_type result;
          result = serializer_type();
          result << SERVICE_ERROR << id << e;
          Send(client, result.data());
        */
        throw e;
      }
      

    } else 
    {
      TODO_FAIL("call type not implemented yet");
    }
  }

  
  thread_manager_ptr_type thread_manager_;
  event_handle_type event_service_start_;
  event_handle_type event_service_stop_;    
  
  std::deque< PendingMessage > messages_;
  fetch::mutex::Mutex message_mutex_;
  std::atomic< bool > running_;

  Protocol* members_[256] = {nullptr}; // TODO: Not thread-safe
  //  std::map<protocol_handler_type, Protocol*> members_;
};
};
};

#endif
