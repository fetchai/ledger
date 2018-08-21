#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <list>
#include <map>

#include "core/serializers/byte_array.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/serializable_exception.hpp"
#include "network/message.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/message_types.hpp"
#include "network/service/protocol.hpp"
#include "network/service/types.hpp"

#include "core/serializers/counter.hpp"
#include "network/service/error_codes.hpp"
#include "network/service/promise.hpp"

namespace fetch {
namespace service {

class ServiceClientInterface
{
  typedef fetch::mutex::Mutex                      subscription_mutex_type;
  typedef std::lock_guard<subscription_mutex_type> subscription_mutex_lock_type;
  class Subscription;
  typedef std::unordered_map<subscription_handler_type, Subscription> subscriptions_type;

public:
  ServiceClientInterface()
    : subscription_mutex_(__LINE__, __FILE__), promises_mutex_(__LINE__, __FILE__)
  {}

  virtual ~ServiceClientInterface() {}

  template <typename... arguments>
  Promise Call(protocol_handler_type const &protocol, function_handler_type const &function,
               arguments &&... args)
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Service Client Calling ", protocol, ":", function);

    Promise         prom;
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

    if (!DeliverRequest(params.data()))
    {
      fetch::logger.Debug("Call failed!");
      prom.reference()->Fail(serializers::SerializableException(
          error::COULD_NOT_DELIVER, byte_array::ConstByteArray("Could not deliver request")));
      promises_mutex_.lock();
      promises_.erase(prom.id());
      promises_mutex_.unlock();
    }

    return prom;
  }

  Promise CallWithPackedArguments(protocol_handler_type const &protocol,
                                  function_handler_type const &function,
                                  byte_array::ByteArray const &args)
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Debug("Service Client Calling (2) ", protocol, ":", function);

    Promise         prom;
    serializer_type params;

    // We're doing the serialise work TWICE to avoid some memory allocations??!?!?
    serializers::SizeCounter<serializer_type> counter;
    counter << SERVICE_FUNCTION_CALL << prom.id();
    PackCallWithPackedArguments(counter, protocol, function, args);

    params.Reserve(counter.size());

    params << SERVICE_FUNCTION_CALL << prom.id();

    promises_mutex_.lock();
    promises_[prom.id()] = prom.reference();
    promises_mutex_.unlock();

    PackCallWithPackedArguments(params, protocol, function, args);

    if (!DeliverRequest(params.data()))
    {
      // HMM(KLL) - I suspect we should kill all the other promises as well here.
      fetch::logger.Debug("Call failed!");
      prom.reference()->Fail(serializers::SerializableException(
          error::COULD_NOT_DELIVER, byte_array::ConstByteArray("Could not deliver request")));
    }

    return prom;
  }

  subscription_handler_type Subscribe(protocol_handler_type const &protocol,
                                      feed_handler_type const &feed, AbstractCallable *callback)
  {
    LOG_STACK_TRACE_POINT;
    fetch::logger.Info("PubSub: SUBSCRIBE ", int(protocol), ":", int(feed));

    subscription_handler_type subid = CreateSubscription(protocol, feed, callback);
    serializer_type           params;

    serializers::SizeCounter<serializer_type> counter;
    counter << SERVICE_SUBSCRIBE << protocol << feed << subid;
    params.Reserve(counter.size());

    params << SERVICE_SUBSCRIBE << protocol << feed << subid;
    DeliverRequest(params.data());
    return subid;
  }

  void Unsubscribe(subscription_handler_type id)
  {
    fetch::logger.Info("PubSub: Unsub ", int(id));
    LOG_STACK_TRACE_POINT;
    Subscription sub;
    {
      subscription_mutex_lock_type lock(subscription_mutex_);
      auto                         subscr = subscriptions_.find(id);
      if (subscr == subscriptions_.end())
      {
        if (std::find(cancelled_subscriptions_.begin(), cancelled_subscriptions_.end(), id) !=
            cancelled_subscriptions_.end())
        {
          fetch::logger.Error("PubSub: Trying to unsubscribe previously cancelled ID ", id);
        }
        else
        {
          fetch::logger.Error("PubSub: Trying to unsubscribe unknown ID ", id);
        }
        return;
      }

      sub = subscriptions_[id];

      cancelled_subscriptions_.push_back(id);
      if (cancelled_subscriptions_.size() > 30)
      {
        cancelled_subscriptions_.pop_front();
      }
      subscriptions_.erase(id);
    }
    if (sub.callback)
    {
      serializer_type params;

      serializers::SizeCounter<serializer_type> counter;
      counter << SERVICE_UNSUBSCRIBE << sub.protocol << sub.feed << id;
      params.Reserve(counter.size());

      params << SERVICE_UNSUBSCRIBE << sub.protocol << sub.feed << id;
      DeliverRequest(params.data());
    }
  }

protected:
  virtual bool DeliverRequest(network::message_type const &) = 0;

  // TODO(?) This isn't connected to anything. This might be why things are exploding.
  void ClearPromises()
  {
    promises_mutex_.lock();
    for (auto &p : promises_)
    {
      p.second->ConnectionFailed();
    }
    promises_.clear();
    promises_mutex_.unlock();
  }

  bool ProcessServerMessage(network::message_type const &msg)
  {
    LOG_STACK_TRACE_POINT;
    bool ret = true;

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
            error::PROMISE_NOT_FOUND, byte_array::ConstByteArray("Could not find promise"));
      }
      promises_mutex_.unlock();

      auto ret = msg.SubArray(params.Tell(), msg.size() - params.Tell());
      it->second->Fulfill(ret);

      promises_mutex_.lock();
      promises_.erase(it);
      promises_mutex_.unlock();
    }
    else if (type == SERVICE_ERROR)
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
            error::PROMISE_NOT_FOUND, byte_array::ConstByteArray("Could not find promise"));
      }

      promises_mutex_.unlock();

      it->second->Fail(e);

      promises_mutex_.lock();
      promises_.erase(it);
      promises_mutex_.unlock();
    }
    else if (type == SERVICE_FEED)
    {
      feed_handler_type         feed;
      subscription_handler_type sub;
      params >> feed >> sub;

      fetch::logger.Info("PubSub: message ", int(feed), ":", int(sub));

      AbstractCallable *cb = 0;
      {
        subscription_mutex_lock_type lock(subscription_mutex_);
        auto                         subscr = subscriptions_.find(sub);
        if (subscr == subscriptions_.end())
        {
          if (std::find(cancelled_subscriptions_.begin(), cancelled_subscriptions_.end(), sub) ==
              cancelled_subscriptions_.end())
          {
            TODO_FAIL("PubSub: We were sent a subscription ID we never allocated:", int(sub));
            return false;
          }
          else
          {
            fetch::logger.Info("PubSub: Ignoring message for old subscription.", int(sub));
            return true;
          }
        }

        if ((*subscr).second.feed != feed)
        {
          TODO_FAIL("PubSub: Subscription's feed ID is different from message feed ID.");
          return false;
        }

        cb = (*subscr).second.callback;
      }

      if (cb)
      {
        serializer_type result;
        try
        {
          (*cb)(result, params);
        }
        catch (serializers::SerializableException const &e)
        {
          e.StackTrace();
          fetch::logger.Error("PubSub: Serialization error: ", e.what());
          throw e;
        }
      }
      else
      {
        fetch::logger.Error("PubSub: Callback is null for feed ", feed, " in subscription ",
                            int(sub));
      }
    }
    else
    {
      ret = false;
    }

    return ret;
  }

private:
  subscription_handler_type CreateSubscription(protocol_handler_type const &protocol,
                                               feed_handler_type const &feed, AbstractCallable *cb)
  {
    LOG_STACK_TRACE_POINT;

    subscription_mutex_lock_type lock(subscription_mutex_);
    subscription_index_counter++;
    subscriptions_[subscription_index_counter] = Subscription(protocol, feed, cb);
    subscription_mutex_.unlock();
    return subscription_handler_type(subscription_index_counter);
  }

  class Subscription
  {
  public:
    Subscription()
    {
      protocol = 0;
      feed     = 0;
      callback = nullptr;
    }
    Subscription(protocol_handler_type protocol, feed_handler_type feed, AbstractCallable *callback)
    {
      this->protocol = protocol;
      this->feed     = feed;
      this->callback = callback;
    }

    Subscription(const Subscription &) = default;
    Subscription(Subscription &&)      = default;
    Subscription &operator=(const Subscription &) = default;
    Subscription &operator=(Subscription &&) = default;

    std::string summarise()
    {
      char  buffer[1000];
      char *p = buffer;
      p += sprintf(p, " Subscription protocol=%d handler=%d callback=%p ", int(protocol), int(feed),
                   ((void *)(callback)));
      return std::string(buffer);
    }

    protocol_handler_type protocol = 0;
    feed_handler_type     feed     = 0;
    AbstractCallable *    callback = nullptr;
  };

  subscriptions_type                   subscriptions_;
  std::list<subscription_handler_type> cancelled_subscriptions_;
  subscription_mutex_type              subscription_mutex_;
  subscription_handler_type            subscription_index_counter;

  std::map<Promise::promise_counter_type, Promise::shared_promise_type> promises_;
  fetch::mutex::Mutex                                                   promises_mutex_;
};
}  // namespace service
}  // namespace fetch
