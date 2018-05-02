#ifndef SERVICE_CLIENT_INTERFACE_HPP
#define SERVICE_CLIENT_INTERFACE_HPP
#include "network/message.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"
#include "service/callable_class_member.hpp"
#include "service/message_types.hpp"
#include "service/protocol.hpp"
#include "service/types.hpp"
#include "serializer/counter.hpp"

#include "service/error_codes.hpp"
#include "service/promise.hpp"

namespace fetch {
namespace service {

class ServiceClientInterface {
 public:
  ServiceClientInterface()
      : subscription_mutex_(__LINE__, __FILE__),
        promises_mutex_(__LINE__, __FILE__) {}

  virtual ~ServiceClientInterface() {}

  template <typename... arguments>
  Promise Call(protocol_handler_type const& protocol,
               function_handler_type const& function, arguments&& ...args) {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Service Client Calling ", protocol, ":", function);

    Promise prom;
    serializer_type params;

    serializers::SizeCounter<serializer_type> counter;
    counter << SERVICE_FUNCTION_CALL << prom.id();

    PackCall(counter, protocol, function, args...);

    params.Reserve(counter.size());

    params << SERVICE_FUNCTION_CALL << prom.id();

    promises_mutex_.lock();
    promises_[prom.id()] = prom.reference();
    promises_mutex_.unlock();

    PackCall(params, protocol, function, std::forward<arguments>(args)...);

    if (!DeliverRequest(params.data())) {
      fetch::logger.Debug("Call failed!");
      prom.reference()->Fail(serializers::SerializableException(
          error::COULD_NOT_DELIVER,
          byte_array::ConstByteArray("Could not deliver request")));
      promises_mutex_.lock();
      promises_.erase(prom.id());
      promises_mutex_.unlock();
    }

    return prom;
  }

  Promise CallWithPackedArguments(protocol_handler_type const& protocol,
                                  function_handler_type const& function,
                                  byte_array::ByteArray const& args) {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Service Client Calling (2) ", protocol, ":", function);

    Promise prom;
    serializer_type params;

    serializers::SizeCounter<serializer_type> counter;
    counter << SERVICE_FUNCTION_CALL << prom.id();
    PackCallWithPackedArguments(counter, protocol, function, args);

    params.Reserve(counter.size());

    params << SERVICE_FUNCTION_CALL << prom.id();

    promises_mutex_.lock();
    promises_[prom.id()] = prom.reference();
    promises_mutex_.unlock();

    PackCallWithPackedArguments(params, protocol, function, args);

    if (!DeliverRequest(params.data())) {
      fetch::logger.Debug("Call failed!");
      prom.reference()->Fail(serializers::SerializableException(
          error::COULD_NOT_DELIVER,
          byte_array::ConstByteArray("Could not deliver request")));
    }

    return prom;
  }

  subscription_handler_type Subscribe(protocol_handler_type const& protocol,
                                      feed_handler_type const& feed,
                                      AbstractCallable* callback) {
    LOG_STACK_TRACE_POINT;

    subscription_handler_type subid =
        CreateSubscription(protocol, feed, callback);
    serializer_type params;

    serializers::SizeCounter<serializer_type> counter;
    counter << SERVICE_SUBSCRIBE << protocol << feed << subid;
    params.Reserve(counter.size());

    params << SERVICE_SUBSCRIBE << protocol << feed << subid;
    DeliverRequest(params.data());
    return subid;
  }

  void Unsubscribe(subscription_handler_type id) {
    LOG_STACK_TRACE_POINT;

    subscription_mutex_.lock();
    auto& sub = subscriptions_[id];

    serializer_type params;

    serializers::SizeCounter<serializer_type> counter;
    counter << SERVICE_UNSUBSCRIBE << sub.protocol << sub.feed << id;
    params.Reserve(counter.size());

    params << SERVICE_UNSUBSCRIBE << sub.protocol << sub.feed << id;
    subscription_mutex_.unlock();

    DeliverRequest(params.data());

    subscription_mutex_.lock();
    delete sub.callback;
    sub.protocol = 0;
    sub.feed = 0;
    subscription_mutex_.unlock();
  }

 protected:
  virtual bool DeliverRequest(network::message_type const&) = 0;

  void ClearPromises() {
    promises_mutex_.lock();
    for (auto& p : promises_) {
      p.second->ConnectionFailed();
    }
    promises_.clear();
    promises_mutex_.unlock();
  }

  bool ProcessServerMessage(network::message_type const& msg) {
    LOG_STACK_TRACE_POINT;
    bool ret = true;

    serializer_type params(msg);

    service_classification_type type;
    params >> type;

    if (type == SERVICE_RESULT) {
      Promise::promise_counter_type id;
      params >> id;

      promises_mutex_.lock();
      auto it = promises_.find(id);
      if (it == promises_.end()) {
        promises_mutex_.unlock();

        throw serializers::SerializableException(
            error::PROMISE_NOT_FOUND,
            byte_array::ConstByteArray("Could not find promise"));
      }
      promises_mutex_.unlock();

      auto ret = msg.SubArray(params.Tell(), msg.size() - params.Tell());
      it->second->Fulfill(ret);

      promises_mutex_.lock();
      promises_.erase(it);
      promises_mutex_.unlock();
    } else if (type == SERVICE_ERROR) {
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
            byte_array::ConstByteArray("Could not find promise"));
      }

      promises_mutex_.unlock();

      it->second->Fail(e);

      promises_mutex_.lock();
      promises_.erase(it);
      promises_mutex_.unlock();

    } else if (type == SERVICE_FEED) {
      feed_handler_type feed;
      subscription_handler_type sub;
      params >> feed >> sub;
      subscription_mutex_.lock();
      if (subscriptions_[sub].feed != feed) {
        fetch::logger.Error("Feed id mismatch ", feed);
        TODO_FAIL("feed id mismatch");
      }

      auto& subde = subscriptions_[sub];
      subscription_mutex_.unlock();

      subde.mutex.lock();
      auto cb = subde.callback;
      subde.mutex.unlock();

      if (cb != nullptr) {
        serializer_type result;
        try {
          (*cb)(result, params);
        } catch (serializers::SerializableException const& e) {
          e.StackTrace();

          fetch::logger.Error("Serialization error: ", e.what());
          throw e;
        }
      } else {
        fetch::logger.Error("Callback is null for feed ", feed);
      }

    } else {
      ret = false;
    }

    return ret;
  }

 private:
  subscription_handler_type CreateSubscription(
      protocol_handler_type const& protocol, feed_handler_type const& feed,
      AbstractCallable* cb) {
    LOG_STACK_TRACE_POINT;

    subscription_mutex_.lock();
    std::size_t i = 0;
    for (; i < 256; ++i) {
      if (subscriptions_[i].callback == nullptr) break;
    }

    if (i >= 256) {
      TODO_FAIL("could not allocate a free space for subscription");
    }

    auto& sub = subscriptions_[i];
    sub.protocol = protocol;
    sub.feed = feed;
    sub.callback = cb;
    subscription_mutex_.unlock();
    return subscription_handler_type(i);
  }

  struct Subscription {
    protocol_handler_type protocol = 0;
    feed_handler_type feed = 0;
    AbstractCallable* callback = nullptr;
    fetch::mutex::Mutex mutex;
  };

  Subscription subscriptions_[256];  // TODO: make centrally configurable;
  fetch::mutex::Mutex subscription_mutex_;

  std::map<Promise::promise_counter_type, Promise::shared_promise_type>
      promises_;
  fetch::mutex::Mutex promises_mutex_;
};
}
}
#endif
