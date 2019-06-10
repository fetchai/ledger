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

#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/service/service_client.hpp"
#include "network/service/types.hpp"

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace fetch {
namespace network {

class SubscriptionsContainer
{
  using mutex_type           = fetch::mutex::Mutex;
  using protocol_number_type = fetch::service::protocol_handler_type;
  using feed_handler_type    = fetch::service::feed_handler_type;
  using subs_handle_type     = uint64_t;
  using client_type          = fetch::service::ServiceClient;
  using client_ptr           = std::shared_ptr<client_type>;

  using client_handle_type = uint64_t;
  using verb_type          = uint64_t;

  static constexpr char const *LOGGING_NAME = "SubscriptionsContainer";

  using existing_subs_type =
      std::map<std::tuple<client_handle_type, verb_type, protocol_number_type>, subs_handle_type>;

public:
  SubscriptionsContainer(const SubscriptionsContainer &rhs) = delete;
  SubscriptionsContainer(SubscriptionsContainer &&rhs)      = delete;
  SubscriptionsContainer operator=(const SubscriptionsContainer &rhs) = delete;
  SubscriptionsContainer operator=(SubscriptionsContainer &&rhs)             = delete;
  bool                   operator==(const SubscriptionsContainer &rhs) const = delete;
  bool                   operator<(const SubscriptionsContainer &rhs) const  = delete;

  SubscriptionsContainer() = default;

  virtual ~SubscriptionsContainer() = default;

  template <class FUNC_CLASS>
  subs_handle_type Subscribe(client_ptr client, protocol_number_type protocol_number,
                             feed_handler_type verb, const std::string &name, FUNC_CLASS *func)
  {
    FETCH_LOCK(mutex_);

    auto identifier = std::make_tuple(client->handle(), protocol_number, verb);

    auto search_result = existing_subs_.find(identifier);

    if (search_result != existing_subs_.end())
    {
      return search_result->second;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "### Subscribing to the protocol...");

    auto r                     = handle_counter_++;
    existing_subs_[identifier] = r;
    subs_[r]                   = CreateSubscription(client, protocol_number, verb, name, func);
    return r;
  }

  void ConnectionDropped(fetch::network::TCPClient::handle_type connection_handle)
  {
    FETCH_LOCK(mutex_);

    auto item = existing_subs_.begin();
    while (item != existing_subs_.end())
    {
      if (std::get<0>(item->first) == connection_handle)
      {
        auto handle = item->second;
        RemoveSubscription(handle);
        item = existing_subs_.erase(item);
      }
      else
      {
        ++item;
      }
    }
  }

  void AssociateName(const std::string &name, client_handle_type connection_handle,
                     protocol_number_type proto = 0, feed_handler_type verb = 0)
  {
    FETCH_LOCK(mutex_);

    for (auto &item : existing_subs_)
    {
      if ((std::get<0>(item.first) == connection_handle) &&
          (!proto || std::get<2>(item.first) == proto) &&
          (!verb || std::get<1>(item.first) == verb))
      {
        subs_[item.second]->name_ = name;
      }
    }
  }

  void VisitSubscriptions(std::function<void(client_ptr)> func)
  {
    std::list<std::shared_ptr<Subscription>> subscriptions_local;

    {
      FETCH_LOCK(mutex_);

      for (auto &subscription : subs_)
      {
        subscriptions_local.push_back(subscription.second);
      }
    }

    for (auto subscription : subscriptions_local)
    {
      func(subscription->GetClient());
    }
  }

  std::vector<std::string> GetAllSubscriptions(protocol_number_type proto, feed_handler_type verb)
  {
    FETCH_LOCK(mutex_);

    std::vector<std::string> r;
    for (auto &item : existing_subs_)
    {
      if ((!proto || std::get<1>(item.first) == proto) &&
          (!verb || std::get<2>(item.first) == verb))
      {
        r.push_back(subs_[item.second]->getName());
      }
    }
    return r;
  }

  bool RemoveSubscription(subs_handle_type handle)
  {
    FETCH_LOCK(mutex_);

    auto location = subs_.find(handle);
    if (location != subs_.end())
    {
      subs_.erase(location);
      return true;
    }
    return false;
  }

private:
  class Subscription
  {
  public:
    friend class SubscriptionsContainer;
    Subscription(client_ptr client, fetch::service::subscription_handler_type handle,
                 std::string name)
      : client_(std::move(client))
      , handle_(handle)
      , name_(std::move(name))
    {}
    virtual ~Subscription()
    {
      client_->Unsubscribe(handle_);
    }
    const std::string &getName()
    {
      return name_;
    }

    client_ptr GetClient()
    {
      return client_;
    }

  private:
    client_ptr                                client_;
    fetch::service::subscription_handler_type handle_;
    std::string                               name_;
  };

  template <class FUNC_CLASS>
  std::shared_ptr<Subscription> CreateSubscription(client_ptr           client,
                                                   protocol_number_type protocol_number,
                                                   feed_handler_type verb, const std::string &name,
                                                   FUNC_CLASS *func)
  {
    auto handle = client->Subscribe(protocol_number, verb, func);
    return std::make_shared<Subscription>(client, handle, name);
  }

  using subs_type                    = std::map<uint64_t, std::shared_ptr<Subscription>>;
  subs_handle_type   handle_counter_ = 0;
  mutex_type         mutex_{__LINE__, __FILE__};
  existing_subs_type existing_subs_;
  subs_type          subs_;
};

}  // namespace network
}  // namespace fetch
