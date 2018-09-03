#include "network/service/client_interface.hpp"

namespace fetch {
namespace service {

ServiceClientInterface::ServiceClientInterface()
  : subscription_mutex_(__LINE__, __FILE__), promises_mutex_(__LINE__, __FILE__)
{}

Promise ServiceClientInterface::CallWithPackedArguments(protocol_handler_type const &protocol,
                                                        function_handler_type const &function,
                                                        byte_array::ByteArray const &args)
{
  LOG_STACK_TRACE_POINT;
  FETCH_LOG_DEBUG(LOGGING_NAME, "Service Client Calling (2) ", protocol, ":", function);

  Promise         prom = MakePromise();
  serializer_type params;

  // We're doing the serialise work TWICE to avoid some memory allocations??!?!?
  serializers::SizeCounter<serializer_type> counter;
  counter << SERVICE_FUNCTION_CALL << prom->id();
  PackCallWithPackedArguments(counter, protocol, function, args);

  params.Reserve(counter.size());

  params << SERVICE_FUNCTION_CALL << prom->id();

  promises_mutex_.lock();
  promises_[prom->id()] = prom;
  promises_mutex_.unlock();

  PackCallWithPackedArguments(params, protocol, function, args);

  if (!DeliverRequest(params.data()))
  {
    // HMM(KLL) - I suspect we should kill all the other promises as    well here.
    FETCH_LOG_DEBUG(LOGGING_NAME, "Call failed!");
    prom->Fail(serializers::SerializableException(
      error::COULD_NOT_DELIVER, byte_array::ConstByteArray("Could not deliver request")));
  }

  return prom;
}

subscription_handler_type ServiceClientInterface::Subscribe(protocol_handler_type const &protocol,
                                                            feed_handler_type const &feed,
                                                            AbstractCallable *callback)
{
  LOG_STACK_TRACE_POINT;
  FETCH_LOG_INFO(LOGGING_NAME, "PubSub: SUBSCRIBE ", int(protocol), ":", int(feed));

  subscription_handler_type subid = CreateSubscription(protocol, feed, callback);
  serializer_type           params;

  serializers::SizeCounter<serializer_type> counter;
  counter << SERVICE_SUBSCRIBE << protocol << feed << subid;
  params.Reserve(counter.size());

  params << SERVICE_SUBSCRIBE << protocol << feed << subid;
  DeliverRequest(params.data());
  return subid;
}

void ServiceClientInterface::Unsubscribe(subscription_handler_type id)
{
  FETCH_LOG_INFO(LOGGING_NAME, "PubSub: Unsub ", int(id));
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
        FETCH_LOG_ERROR(LOGGING_NAME, "PubSub: Trying to unsubscribe previously cancelled ID ", id);
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "PubSub: Trying to unsubscribe unknown ID ", id);
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


// TODO(EJF) This isn't connected to anything. This might be why things are exploding.
void ServiceClientInterface::ClearPromises()
{
#if 0
  promises_mutex_.lock();
  for (auto &p : promises_)
  {
    p.second->ConnectionFailed();
  }
  promises_.clear();
  promises_mutex_.unlock();
#endif
}

bool ServiceClientInterface::ProcessServerMessage(network::message_type const &msg)
{
  LOG_STACK_TRACE_POINT;
  bool ret = true;

  serializer_type params(msg);

  service_classification_type type;
  params >> type;

  if (type == SERVICE_RESULT)
  {
    PromiseCounter id;
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
    PromiseCounter id;
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

    FETCH_LOG_INFO(LOGGING_NAME, "PubSub: message ", int(feed), ":", int(sub));

    AbstractCallable *cb = nullptr;
    {
      subscription_mutex_lock_type lock(subscription_mutex_);
      auto                         subscr = subscriptions_.find(sub);
      if (subscr == subscriptions_.end())
      {
        if (std::find(cancelled_subscriptions_.begin(), cancelled_subscriptions_.end(), sub) ==
            cancelled_subscriptions_.end())
        {
          FETCH_LOG_ERROR(LOGGING_NAME,
                          "PubSub:  We were sent a subscription ID we never allocated: ", int(sub));
          return false;
        }
        else
        {
          FETCH_LOG_INFO(LOGGING_NAME, "PubSub: Ignoring message for old subscription.", int(sub));
          return true;
        }
      }

      if ((*subscr).second.feed != feed)
      {
        FETCH_LOG_ERROR(LOGGING_NAME,
                        "PubSub: Subscription's feed ID is different from message feed ID.");
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
        FETCH_LOG_ERROR(LOGGING_NAME, "PubSub: Serialization error: ", e.what());
        throw e;
      }
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "PubSub: Callback is null for feed ", feed, " in subscription ",
                      int(sub));
    }
  }
  else
  {
    ret = false;
  }

  return ret;
}

subscription_handler_type
ServiceClientInterface::CreateSubscription(protocol_handler_type const &protocol,
                                           feed_handler_type const &feed, AbstractCallable *cb)
{
  LOG_STACK_TRACE_POINT;

  subscription_mutex_lock_type lock(subscription_mutex_);
  subscription_index_counter++;
  subscriptions_[subscription_index_counter] = Subscription(protocol, feed, cb);
  subscription_mutex_.unlock();
  return subscription_handler_type(subscription_index_counter);
}


} // namespace service
} // namespace fetch