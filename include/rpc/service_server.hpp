#ifndef RPC_SERVICE_SERVER_HPP
#define RPC_SERVICE_SERVER_HPP

#include "rpc/callable_class_member.hpp"
#include "rpc/message_types.hpp"
#include "rpc/protocol.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

#include "rpc/error_codes.hpp"
#include "rpc/promise.hpp"
#include "mutex.hpp"

#include "assert.hpp"
#include "network/network_server.hpp"

#include <map>
#include <atomic>
#include <mutex>
#include <deque>

namespace fetch {
namespace rpc {

class ServiceServer : public network::NetworkServer {
 public:
  struct PendingMessage {
    handle_type client;
    network::message_type message;
  };
  typedef byte_array::ConstByteArray byte_array_type;

  ServiceServer(uint16_t port) : NetworkServer(port), message_mutex_(__LINE__, __FILE__) {
    running_ = false;
  }

  void Start() override {
    if( worker_thread_ == nullptr) {
      running_ = true;      
      NetworkServer::Start();      
      worker_thread_ = new std::thread([this]() {
          this->ProcessMessages();
        });
    }
  }

  void Stop() override {    
    if( worker_thread_ != nullptr) {
      NetworkServer::Stop();
      running_ = false;
      worker_thread_->join();
      delete worker_thread_;
    }
  }
  
  void PushRequest(handle_type client,
                   network::message_type const& msg) override {
    PendingMessage pm = {client, msg};
    std::lock_guard< fetch::mutex::Mutex > lock(message_mutex_);
    messages_.push_back(pm);
  }

  void Add(protocol_handler_type const& name, Protocol* protocol) {
    if(members_[name] != nullptr) {
      throw serializers::SerializableException(
          error::PROTOCOL_EXISTS,
          byte_array_type("Member already exists: "));
    }

    members_[name] = protocol;

    for(auto &feed: protocol->feeds()) {
      feed->AttachToService( this );
    }
  }

 private:
  void Call(serializer_type& result, serializer_type params) {
    protocol_handler_type protocol;
    function_handler_type function;
    params >> protocol >> function;

    if(members_[protocol] == nullptr) {
      throw serializers::SerializableException(
          error::PROTOCOL_NOT_FOUND,
          byte_array_type("Could not find protocol: "));
    }

    auto& mod = *members_[protocol] ; //*it->second;
    return mod[function](result, params);
  }

  void ProcessMessages() {
    while(running_) {
      
      message_mutex_.lock();
      bool has_messages = (!messages_.empty());
      message_mutex_.unlock();
      
      while(has_messages) {
        message_mutex_.lock();


        PendingMessage pm;
        has_messages = (!messages_.empty());
        if(has_messages) { // To ensure we can make a worker pool in the future
          pm = messages_.front();
          messages_.pop_front();
        };
        message_mutex_.unlock();
        
        if(has_messages) {
          ProcessClientMessage( pm.client, pm.message );
        }
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    }
  }


  void ProcessClientMessage(handle_type client,
                   network::message_type const& msg) {
    serializer_type params(msg);

    rpc_classification_type type;
    params >> type;

    if (type == RPC_FUNCTION_CALL) {
      serializer_type result;
      Promise::promise_counter_type id;
      
      try {
        params >> id;
        result << RPC_RESULT << id;
        Call(result, params);
      } catch (serializers::SerializableException const& e) {
        result = serializer_type();
        result << RPC_ERROR << id << e;
      }

      Send(client, result.data());
    } else if  (type == RPC_SUBSCRIBE)  {
      protocol_handler_type protocol;
      feed_handler_type feed;
      subscription_handler_type subid;
      
      try {
        params >> protocol >> feed >> subid;
        auto& mod = *members_[protocol] ;      

        mod.Subscribe( client, feed, subid );
        
      } catch (serializers::SerializableException const& e) {
        // FIX Serialization of errors such that this also works
        /*
        serializer_type result;
        result = serializer_type();
        result << RPC_ERROR << id << e;
        Send(client, result.data());
        */
        throw e;
      }
      
    } else if  (type == RPC_UNSUBSCRIBE)  {
      protocol_handler_type protocol;
      feed_handler_type feed;
      subscription_handler_type subid;
      
      try {
        params >> protocol >> feed >> subid;
        auto& mod = *members_[protocol] ;      

        mod.Unsubscribe( client, feed, subid );
        
      } catch (serializers::SerializableException const& e) {
        // FIX Serialization of errors such that this also works
        /*
        serializer_type result;
        result = serializer_type();
        result << RPC_ERROR << id << e;
        Send(client, result.data());
        */
        throw e;
      }
      

    } else {
      TODO_FAIL("call type not implemented yet");
    }
  }
  
  std::deque< PendingMessage > messages_;
  fetch::mutex::Mutex message_mutex_;
  std::atomic< bool > running_;
  std::thread *worker_thread_ = nullptr;

  Protocol* members_[256] = {nullptr}; // TODO: Not thread-safe
  //  std::map<protocol_handler_type, Protocol*> members_;
};
};
};

#endif
