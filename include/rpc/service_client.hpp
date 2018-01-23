#ifndef RPC_SERVICE_CLIENT_HPP
#define RPC_SERVICE_CLIENT_HPP

#include "rpc/callable_class_member.hpp"
#include "rpc/message_types.hpp"
#include "rpc/protocol.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

#include "rpc/error_codes.hpp"
#include "rpc/promise.hpp"

#include "assert.hpp"
#include "network/network_client.hpp"
#include "mutex.hpp"

#include <map>

namespace fetch {
namespace rpc {

class ServiceClient : public network::NetworkClient {
 public:


  ServiceClient(std::string const& host, uint16_t const& port)
      : NetworkClient(host, port) {
    running_ = true;
    worker_thread_ = new std::thread([this]() {
        this->ProcessMessages();
      });
  }

  ~ServiceClient() {
    running_ = false;
    worker_thread_->join();
    delete worker_thread_;    
  }

  template <typename... arguments>
  Promise Call(protocol_handler_type const& protocol,
               function_handler_type const& function, arguments... args) {
    Promise prom;
    serializer_type params;
    params << RPC_FUNCTION_CALL << prom.id();

    promises_mutex_.lock();
    promises_[prom.id()] = prom.reference();
    promises_mutex_.unlock();
      
    PackCall(params, protocol, function, args...);
    Send(params.data());

    return prom;
  }

  subscription_handler_type Subscribe(protocol_handler_type const& protocol,
                                      feed_handler_type const& feed,
                                      AbstractCallable *callback) {
    subscription_handler_type subid = CreateSubscription( protocol, feed, callback );
    serializer_type params;
    params << RPC_SUBSCRIBE << protocol << feed << subid ;
    Send(params.data());
    return subid;
  }
  
  void Unsubscribe(subscription_handler_type id) {
    subscription_mutex_.lock();    
    auto &sub = subscriptions_[id];
    
    serializer_type params;
    params << RPC_UNSUBSCRIBE << sub.protocol << sub.feed << id;
    subscription_mutex_.unlock();
    
    Send(params.data());
    
    subscription_mutex_.lock();
    delete sub.callback;
    sub.protocol = 0;
    sub.feed = 0;
    subscription_mutex_.unlock();
  }  
  
  void PushMessage(network::message_type const& msg) override {
    std::lock_guard< fetch::mutex::Mutex > lock(message_mutex_);
    messages_.push_back(msg);
  }

 private:
  subscription_handler_type CreateSubscription(protocol_handler_type const& protocol,
                                               feed_handler_type const& feed,
                                               AbstractCallable *cb) {
    subscription_mutex_.lock();
    std::size_t i = 0;
    for(; i < 256; ++i) {
      if(subscriptions_[i].callback == nullptr) 
        break;      
    }
    
    if(i >= 256) {
      TODO_FAIL("could not allocate a free space for subscription");
    }
    
    auto &sub = subscriptions_[i];
    sub.protocol = protocol;
    sub.feed = feed;    
    sub.callback = cb;
    subscription_mutex_.unlock();
    return i;
  }
  

  void ProcessMessages() {
    while(running_) {
      
      message_mutex_.lock();
      bool has_messages = (!messages_.empty());
      message_mutex_.unlock();
      
      while(has_messages) {
        message_mutex_.lock();

        network::message_type msg;
        has_messages = (!messages_.empty());
        if(has_messages) {
          msg = messages_.front();
          messages_.pop_front();
        };
        message_mutex_.unlock();
        
        if(has_messages) {
          ProcessServerMessage( msg );
        }
      }

      std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    }
  }
  
  void ProcessServerMessage(network::message_type const& msg) {
    serializer_type params(msg);

    rpc_classification_type type;
    params >> type;

    if (type == RPC_RESULT) {
      Promise::promise_counter_type id;
      params >> id;


      promises_mutex_.lock();
      auto it = promises_.find(id);
      if (it == promises_.end()) {
        promises_mutex_.unlock();

        throw serializers::SerializableException(
            error::PROMISE_NOT_FOUND,
            "Could not find promise");
      }
      promises_mutex_.unlock();
        
      auto ret = msg.SubArray(params.Tell(), msg.size() - params.Tell());
      it->second->Fulfill(ret.Copy());

      promises_mutex_.lock();
      promises_.erase(it);
      promises_mutex_.unlock();
    } else if (type == RPC_ERROR) {
      Promise::promise_counter_type id;
      params >> id;
      
      serializers::SerializableException e;
      params >> e;

      promises_mutex_.lock();
      auto it = promises_.find(id);
      if (it == promises_.end()) {
        promises_mutex_.unlock();        
        throw serializers::SerializableException(
            error::PROMISE_NOT_FOUND,
            "Could not find promise");
      }
      promises_mutex_.unlock();
    
      it->second->Fail(e);

      promises_mutex_.lock();
      promises_.erase(it);
      promises_mutex_.unlock();
      
    } else if( type == RPC_FEED ) {
      feed_handler_type feed;
      subscription_handler_type sub;
      params >> feed >> sub;
      subscription_mutex_.lock();
      if( subscriptions_[sub].feed != feed) {
        TODO_FAIL("feed id mismatch");
      }

      auto &subde = subscriptions_[sub];
      subscription_mutex_.unlock(); 

      subde.mutex.lock();
      // TODO: Locking should be done on an per, callback basis rather than global
      auto cb = subde.callback;

      if(cb!=nullptr) {
        serializer_type result;
        (*cb)(result, params);
      } else {
        TODO_FAIL("callback is null");
      }
        
      subde.mutex.unlock();
    } else {
      throw serializers::SerializableException(
          error::UNKNOWN_MESSAGE, "Unknown message");
    }
  }

  struct Subscription {
    protocol_handler_type protocol = 0;
    feed_handler_type feed = 0;
    AbstractCallable* callback = nullptr;
    fetch::mutex::Mutex mutex;
  };
  
  Subscription subscriptions_[256];
  fetch::mutex::Mutex subscription_mutex_;



  std::map<Promise::promise_counter_type, Promise::shared_promise_type>
      promises_;
  fetch::mutex::Mutex promises_mutex_;

  std::atomic< bool > running_;
  std::deque< network::message_type > messages_;
  fetch::mutex::Mutex message_mutex_;  
  std::thread *worker_thread_ = nullptr;  
};
};
};

#endif
