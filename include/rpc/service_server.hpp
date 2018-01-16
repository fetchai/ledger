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
  typedef byte_array::ReferencedByteArray byte_array_type;

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

  void Add(byte_array_type const& name, Protocol* fnc) {
    if (members_.find(name) != members_.end()) {
      throw serializers::SerializableException(
          error::PROTOCOL_EXISTS,
          byte_array_type("Member already exists: ") + name);
    }

    members_[name] = fnc;
  }

 private:
  void Call(serializer_type& result, serializer_type params) {
    byte_array_type protocol, function;
    params >> protocol >> function;

    auto it = members_.find(protocol);
    if (it == members_.end()) {
      throw serializers::SerializableException(
          error::PROTOCOL_NOT_FOUND,
          byte_array_type("Could not find protocol: ") + protocol);
    }

    auto& mod = *it->second;
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
        std::cout << "Got message" << std::endl;
        has_messages = (!messages_.empty());
        if(has_messages) { // To ensure we can make a worker pool in the future
          pm = messages_.front();
          messages_.pop_front();
        };
        message_mutex_.unlock();
        
        if(has_messages) {
          std::cout << "Processing message" << std::endl;
          ProcessClientMessage( pm.client, pm.message );
        }
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
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
      params >> id;
      try {
        result << RPC_RESULT << id;
        Call(result, params);
      } catch (serializers::SerializableException const& e) {
        result = serializer_type();
        result << RPC_ERROR << id << e;
      }

      Respond(client, result.data());
    } else {
      TODO_FAIL("call type not implemented yet");
    }
  }
  
  std::deque< PendingMessage > messages_;
  fetch::mutex::Mutex message_mutex_;
  std::atomic< bool > running_;
  std::thread *worker_thread_ = nullptr;
  
  std::map<byte_array_type, Protocol*> members_;
};
};
};

#endif
