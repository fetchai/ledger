#pragma once
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

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "core/assert.hpp"
#include "core/mutex.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/error_codes.hpp"
#include "network/service/feed_subscription_manager.hpp"
#include "network/service/types.hpp"

namespace fetch {
namespace service {

class FeedSubscriptionManager;

/* A class that defines a generic protocol.
 *
 * This class is used for defining a general protocol with
 * remote-function-calls (SERVICEs) and data feeds. The SERVICEs are defined
 * from a C++ function signature using any sub class of
 * <service::AbstractCallable> including <service::Function> and
 * <service::CallableClassMember>. The feeds are available from any
 * functionality implementation that sub-classes
 * <service::HasPublicationFeed>.
 *
 * A current limitation of the implementation is that there is only
 * support for 256 SERVICE functions. It the next version of this class,
 * this should be changed to be variable and allocated at construction
 * time.
 *
 * TODO(issue 21):
 */
class Protocol
{
public:
  using callable_type          = AbstractCallable *;
  using stored_type            = std::shared_ptr<AbstractCallable>;
  using byte_array_type        = byte_array::ConstByteArray;
  using connection_handle_type = typename network::AbstractConnection::connection_handle_type;
  using middleware_type =
      std::function<void(connection_handle_type const &, byte_array::ByteArray const &)>;

  static constexpr char const *LOGGING_NAME = "Protocol";

  Protocol()          = default;
  virtual ~Protocol() = default;

  /* Operator to access the different functions in the protocol.
   * @n is the idnex of callable in the protocol.
   *
   * The result of this operator is a <callable_type> that can be
   * invoked in accodance with the definition of an
   * <service::AbstractCallable>.
   *
   * This operator throws a <SerializableException> if the index does
   * not map to a callable
   *
   * @return a reference to the call.
   */
  callable_type operator[](function_handler_type const &n)
  {
    LOG_STACK_TRACE_POINT;

    auto iter = members_.find(n);
    if (iter == members_.end())
    {
      DumpMemberTable();

      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to lookup function handler: ", n);

      throw serializers::SerializableException(
          error::MEMBER_NOT_FOUND, byte_array_type("Could not find protocol member function"));
    }
    return iter->second.get();
  }

  /* Exposes a function or class member function.
   * @n is a unique identifier for the callable being exposed
   * @instance is instance of an implementation.
   * @fnc is a pointer to the callable function.
   *
   * The pointer provided is used to invoke the callable when a call
   * matching the identifier is recieved by a service.
   *
   * In the next implementation of this, one should use unique_ptr
   * rather than a raw pointer. This will have no impact on the rest of
   * the code as it is always a reference return and not the raw pointer
   *
   * TODO(issue 21):
   */
  template <typename C, typename R, typename... Args>
  void Expose(function_handler_type const &n, C *instance, R (C::*function)(Args...))
  {
    stored_type fnc(new service::CallableClassMember<C, R(Args...)>(instance, function));

    auto iter = members_.find(n);
    if (iter != members_.end())
    {
      throw serializers::SerializableException(
          error::MEMBER_EXISTS, byte_array_type("Protocol member function already exists: "));
    }

    members_[n] = fnc;
  }

  template <typename C, typename R, typename... Args>
  void ExposeWithClientArg(function_handler_type const &n, C *instance, R (C::*function)(Args...))
  {
    stored_type fnc(new service::CallableClassMember<C, R(Args...), 1>(Callable::CLIENT_ID_ARG,
                                                                       instance, function));

    auto iter = members_.find(n);
    if (iter != members_.end())
    {
      throw serializers::SerializableException(
          error::MEMBER_EXISTS, byte_array_type("Protocol member function already exists: "));
    }

    members_[n] = fnc;
  }

  template <typename C, typename R, typename... Args>
  void ExposeWithClientContext(function_handler_type const &n, C *instance,
                               R (C::*function)(Args...))
  {
    stored_type fnc(new service::CallableClassMember<C, R(Args...), 1>(Callable::CLIENT_CONTEXT_ARG,
                                                                       instance, function));

    auto iter = members_.find(n);
    if (iter != members_.end())
    {
      throw serializers::SerializableException(
          error::MEMBER_EXISTS, byte_array_type("Protocol member function already exists: "));
    }

    members_[n] = fnc;
  }

  virtual void ConnectionDropped(connection_handle_type /*connection_handle*/)
  {}

  /* Registers a feed from an implementation.
   * @feed is the unique feed identifier.
   * @publisher is a class that subclasses <AbstractPublicationFeed>.
   *
   */
  void RegisterFeed(feed_handler_type const &feed, AbstractPublicationFeed *publisher)
  {
    LOG_STACK_TRACE_POINT;

    feeds_.push_back(std::make_shared<FeedSubscriptionManager>(feed, publisher));
  }

  /* Subscribe client to feed.
   * @client is the client id.
   * @feed is the feed identifier.
   * @id is the subscription id allocated on the client side.
   *
   * This function is intended to be used by the service to subscribe
   * its clients to the feed.
   */
  void Subscribe(uint64_t client,  // TODO(issue 21): Standardize client type over the code.
                 feed_handler_type const &feed, subscription_handler_type const &id)
  {
    LOG_STACK_TRACE_POINT;

    FETCH_LOG_DEBUG(LOGGING_NAME, "Making subscription for ", client, " ", feed, " ", id);

    feeds_mutex_.lock();
    std::size_t i = 0;
    for (; i < feeds_.size(); ++i)
    {
      if (feeds_[i]->feed() == feed)
      {
        break;
      }
    }
    if (i == feeds_.size())
    {
      TODO_FAIL("make serializable error, feed not found");
    }
    auto &f = feeds_[i];
    feeds_mutex_.unlock();

    f->Subscribe(client, id);
  }

  /* Unsubscribe client to feed.
   * @client is the client id.
   * @feed is the feed identifier.
   * @id is the subscription id allocated on the client side.
   *
   * This function is intended to be used by the service to unsubscribe
   * its clients to the feed.
   */
  void Unsubscribe(uint64_t client,  // TODO(issue 21): Standardize client type over the code.
                   feed_handler_type const &feed, subscription_handler_type const &id)
  {
    LOG_STACK_TRACE_POINT;

    feeds_mutex_.lock();
    std::size_t i = 0;
    for (; i < feeds_.size(); ++i)
    {
      if (feeds_[i]->feed() == feed)
      {
        break;
      }
    }
    if (i == feeds_.size())
    {
      TODO_FAIL("make serializable error, feed not found");
    }
    auto &f = feeds_[i];
    feeds_mutex_.unlock();

    f->Unsubscribe(client, id);
  }

  /* Access memeber to the feeds in the protocol.
   *
   * @return a reference to the feeds.
   */
  std::vector<std::shared_ptr<FeedSubscriptionManager>> &feeds()
  {
    return feeds_;
  }

  void AddMiddleware(middleware_type const &m)
  {
    middleware_.push_back(m);
  }

  void ApplyMiddleware(connection_handle_type const &id, byte_array::ByteArray const &msg)
  {
    for (auto &m : middleware_)
    {
      m(id, msg);
    }
  }

  void DumpMemberTable()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Contents of function table");
    for (auto const &entry : members_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Entry: ", entry.first,
                     " valid: ", static_cast<bool>(entry.second));
    }
  }

private:
  std::vector<middleware_type> middleware_;

  std::map<function_handler_type, stored_type>          members_;
  std::vector<std::shared_ptr<FeedSubscriptionManager>> feeds_;
  fetch::mutex::Mutex                                   feeds_mutex_{__LINE__, __FILE__};
};
}  // namespace service
}  // namespace fetch
