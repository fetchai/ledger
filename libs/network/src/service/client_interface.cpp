//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "network/service/client_interface.hpp"

#include <algorithm>

namespace fetch {
namespace service {

ServiceClientInterface::ServiceClientInterface()
  : subscription_mutex_(__LINE__, __FILE__)
  , promises_mutex_(__LINE__, __FILE__)
{}

Promise ServiceClientInterface::CallWithPackedArguments(protocol_handler_type const &protocol,
                                                        function_handler_type const &function,
                                                        byte_array::ByteArray const &args)
{
  LOG_STACK_TRACE_POINT;
  FETCH_LOG_DEBUG(LOGGING_NAME, "Service Client Calling (2) ", protocol, ":", function);

  Promise         prom = MakePromise();
  serializer_type params;

  serializers::SizeCounter counter;
  counter << SERVICE_FUNCTION_CALL << prom->id();
  PackCallWithPackedArguments(counter, protocol, function, args);

  params.Reserve(counter.size());

  params << SERVICE_FUNCTION_CALL << prom->id();

  FETCH_LOG_DEBUG(LOGGING_NAME, "Registering promise ", prom->id(), " with ", protocol, ':',
                  function, " (packed)", &promises_);

  AddPromise(prom);

  PackCallWithPackedArguments(params, protocol, function, args);

  if (!DeliverRequest(params.data()))
  {
    // HMM(KLL) - I suspect we should kill all the other promises as    well here.
    FETCH_LOG_DEBUG(LOGGING_NAME, "Call failed!");
    prom->Fail(serializers::SerializableException(
        error::COULD_NOT_DELIVER,
        byte_array::ConstByteArray("Could not deliver request in " __FILE__)));
  }

  return prom;
}

subscription_handler_type ServiceClientInterface::Subscribe(protocol_handler_type const &protocol,
                                                            feed_handler_type const &    feed,
                                                            AbstractCallable *           callback)
{
  LOG_STACK_TRACE_POINT;
  FETCH_LOG_INFO(LOGGING_NAME, "PubSub: SUBSCRIBE ", int(protocol), ":", int(feed));

  subscription_handler_type subid = CreateSubscription(protocol, feed, callback);
  serializer_type           params;

  serializers::SizeCounter counter;
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

    serializers::SizeCounter counter;
    counter << SERVICE_UNSUBSCRIBE << sub.protocol << sub.feed << id;
    params.Reserve(counter.size());

    params << SERVICE_UNSUBSCRIBE << sub.protocol << sub.feed << id;
    DeliverRequest(params.data());
  }
}

void ServiceClientInterface::ProcessRPCResult(network::message_type const &msg,
                                              service::serializer_type &   params)
{
  LOG_STACK_TRACE_POINT;
  PromiseCounter id;
  params >> id;

  Promise p = ExtractPromise(id);

  auto ret = msg.SubArray(params.tell(), msg.size() - params.tell());
  p->Fulfill(ret);

  FETCH_LOG_DEBUG(LOGGING_NAME, "ProcessRPCResult: Binning promise ", id,
                  " due to finishing delivering the response");
}

bool ServiceClientInterface::ProcessServerMessage(network::message_type const &msg)
{
  LOG_STACK_TRACE_POINT;
  bool ret = true;

  serializer_type params(msg);

  service_classification_type type;
  params >> type;

  if ((type == SERVICE_RESULT) || (type == 0))
  {
    ProcessRPCResult(msg, params);
  }
  else if (type == SERVICE_ERROR)
  {
    PromiseCounter id;
    params >> id;

    serializers::SerializableException e;
    params >> e;

    // lookup the promise and fail it
    Promise p = ExtractPromise(id);
    p->Fail(e);

    FETCH_LOG_DEBUG(LOGGING_NAME, "Binning promise ", id,
                    " due to finishing delivering the response (error)");
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
    PromiseCounter id;
    params >> id;
    FETCH_LOG_WARN(LOGGING_NAME, " type not recognised ", type, "  promise=", id);
    ret = false;
  }

  return ret;
}

void ServiceClientInterface::AddPromise(Promise const &promise)
{
  FETCH_LOCK(promises_mutex_);
  promises_[promise->id()] = promise;
}

Promise ServiceClientInterface::LookupPromise(PromiseCounter id)
{
  FETCH_LOCK(promises_mutex_);

  auto it = promises_.find(id);
  if (it == promises_.end())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to locate promise with ID: ", id);

    throw serializers::SerializableException(error::PROMISE_NOT_FOUND,
                                             byte_array::ConstByteArray("Could not find promise"));
  }

  return it->second;
}

Promise ServiceClientInterface::ExtractPromise(PromiseCounter id)
{
  FETCH_LOCK(promises_mutex_);

  auto it = promises_.find(id);
  if (it == promises_.end())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to locate promise with ID: ", id);

    throw serializers::SerializableException(error::PROMISE_NOT_FOUND,
                                             byte_array::ConstByteArray("Could not find promise"));
  }

  Promise promise = it->second;
  promises_.erase(it);

  return promise;
}

void ServiceClientInterface::RemovePromise(PromiseCounter id)
{
  FETCH_LOCK(promises_mutex_);
  promises_.erase(id);
}

subscription_handler_type ServiceClientInterface::CreateSubscription(
    protocol_handler_type const &protocol, feed_handler_type const &feed, AbstractCallable *cb)
{
  LOG_STACK_TRACE_POINT;

  subscription_mutex_lock_type lock(subscription_mutex_);
  subscription_index_counter_++;
  subscriptions_[subscription_index_counter_] = Subscription(protocol, feed, cb);
  subscription_mutex_.unlock();
  return subscription_handler_type(subscription_index_counter_);
}

}  // namespace service
}  // namespace fetch
