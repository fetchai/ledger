#ifndef SERVICE_SERVICE_CLIENT_HPP
#define SERVICE_SERVICE_CLIENT_HPP

#include "service/callable_class_member.hpp"
#include "service/message_types.hpp"
#include "service/protocol.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

#include "service/error_codes.hpp"
#include "service/promise.hpp"

#include "assert.hpp"
#include "network/tcp_client.hpp"
#include "mutex.hpp"

#include <map>

namespace fetch 
{
namespace service 
{

template< typename T >  
class ServiceClient : public T 
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
    thread_manager_(thread_manager)
  
  {
    running_ = true;


    // TODO: Replace with thread manager
    worker_thread_ = new std::thread([this]() 
      {
        this->ProcessMessages();
      });
  }

  ~ServiceClient() 
  {
    
    // TODO: Move to OnStop
    running_ = false;
    worker_thread_->join();
    delete worker_thread_; 
       
  }

  template <typename... arguments>
  Promise Call(protocol_handler_type const& protocol,
    function_handler_type const& function, arguments... args) 
  {
    Promise prom;
    serializer_type params;
    params << SERVICE_FUNCTION_CALL << prom.id();

    promises_mutex_.lock();
    promises_[prom.id()] = prom.reference();
    promises_mutex_.unlock();
      
    PackCall(params, protocol, function, args...);
    super_type::Send(params.data());

    return prom;
  }

  subscription_handler_type Subscribe(protocol_handler_type const& protocol,
    feed_handler_type const& feed,
    AbstractCallable *callback) 
  {
    subscription_handler_type subid = CreateSubscription( protocol, feed, callback );
    serializer_type params;
    params << SERVICE_SUBSCRIBE << protocol << feed << subid ;
    super_type::Send(params.data());
    return subid;
  }
  
  void Unsubscribe(subscription_handler_type id) 
  {
    subscription_mutex_.lock();    
    auto &sub = subscriptions_[id];
    
    serializer_type params;
    params << SERVICE_UNSUBSCRIBE << sub.protocol << sub.feed << id;
    subscription_mutex_.unlock();
    
    super_type::Send(params.data());
    
    subscription_mutex_.lock();
    delete sub.callback;
    sub.protocol = 0;
    sub.feed = 0;
    subscription_mutex_.unlock();
  }  
  
  void PushMessage(network::message_type const& msg) override 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(message_mutex_);
    messages_.push_back(msg);
  }
  
  void ConnectionFailed() override
  {
    promises_mutex_.lock();
    for(auto &p: promises_)
    {
      p.second->ConnectionFailed();      
    }
    promises_.clear();    
    promises_mutex_.unlock();    
  }
  

private:
  subscription_handler_type CreateSubscription(protocol_handler_type const& protocol,
    feed_handler_type const& feed,
    AbstractCallable *cb) 
  {
    subscription_mutex_.lock();
    std::size_t i = 0;
    for(; i < 256; ++i) 
    {
      if(subscriptions_[i].callback == nullptr) 
        break;      
    }
    
    if(i >= 256) 
    {
      TODO_FAIL("could not allocate a free space for subscription");
    }
    
    auto &sub = subscriptions_[i];
    sub.protocol = protocol;
    sub.feed = feed;    
    sub.callback = cb;
    subscription_mutex_.unlock();
    return i;
  }
  

  void ProcessMessages() 
  {
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
  
  void ProcessServerMessage(network::message_type const& msg) 
  {
    serializer_type params(msg);

    service_classification_type type;
    params >> type;

    if (type == SERVICE_RESULT) 
    {
      Promise::promise_counter_type id;
      params >> id;


      promises_mutex_.lock();
      auto it = promises_.find(id);
      if (it == promises_.end()) 
      {
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
    } else if (type == SERVICE_ERROR) 
    {
      Promise::promise_counter_type id;
      params >> id;
      
      serializers::SerializableException e;
      params >> e;

      promises_mutex_.lock();
      auto it = promises_.find(id);
      if (it == promises_.end()) 
      {
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
      
    } else if( type == SERVICE_FEED ) 
    {
      feed_handler_type feed;
      subscription_handler_type sub;
      params >> feed >> sub;
      subscription_mutex_.lock();
      if( subscriptions_[sub].feed != feed) 
      {
        TODO_FAIL("feed id mismatch");
      }

      auto &subde = subscriptions_[sub];
      subscription_mutex_.unlock(); 

      subde.mutex.lock();
      // TODO: Locking should be done on an per, callback basis rather than global
      auto cb = subde.callback;

      if(cb!=nullptr) 
      {
        serializer_type result;
        (*cb)(result, params);
      } else 
      {
        TODO_FAIL("callback is null");
      }
        
      subde.mutex.unlock();
    } else 
    {
      throw serializers::SerializableException(
        error::UNKNOWN_MESSAGE, "Unknown message");
    }
  }

  struct Subscription 
  {
    protocol_handler_type protocol = 0;
    feed_handler_type feed = 0;
    AbstractCallable* callback = nullptr;
    fetch::mutex::Mutex mutex;
  };

  thread_manager_ptr_type thread_manager_;

  Subscription subscriptions_[256]; // TODO: make centrally configurable;
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
