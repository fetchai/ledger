#ifndef SUBSCRIPTIONS_CONTAINER_HPP
#define SUBSCRIPTIONS_CONTAINER_HPP

#include <iostream>
#include <string>

namespace fetch
{
namespace network
{

class SubscriptionsContainer
{
using mutex_type = std::mutex;
using lock_type = std::lock_guard<mutex_type>;
using protocol_number_type = uint32_t;
using protocol_handler_type = fetch::service::protocol_handler_type;
using feed_handler_type = fetch::service::feed_handler_type;
using subscription_handle_type = uint64_t;
using client_type = fetch::service::ServiceClient;
using client_ptr = std::shared_ptr<client_type>;

  using existing_subscriptions_type = std::map<std::tuple<uint64_t, uint64_t, uint64_t> ,subscription_handle_type>;
public:
  SubscriptionsContainer(const SubscriptionsContainer &rhs)            = delete;
  SubscriptionsContainer(SubscriptionsContainer &&rhs)                 = delete;
  SubscriptionsContainer operator=(const SubscriptionsContainer &rhs)  = delete;
  SubscriptionsContainer operator=(SubscriptionsContainer &&rhs)       = delete;
  bool operator==(const SubscriptionsContainer &rhs) const = delete;
  bool operator<(const SubscriptionsContainer &rhs) const  = delete;

  explicit SubscriptionsContainer()
  {
  }

  virtual ~SubscriptionsContainer()
  {
  }

  template<class FUNC_CLASS>
  subscription_handle_type Subscribe(
    client_ptr client,
    protocol_handler_type protocol_number,
    feed_handler_type verb,
    FUNC_CLASS *func
    )
  {
    lock_type lock(mutex);
    auto identifier = std::make_tuple(client -> handle(), protocol_number, verb);

    auto search_result = existing_subscriptions.find(identifier);

    if (search_result != existing_subscriptions.end())
    {
      return search_result -> second;
    }

    auto r = handle_counter++;
    existing_subscriptions[identifier] = r;
    subscriptions[r] = CreateSubscription(client, protocol_number, verb, func);
    return r;
  }

  void ConnectionDropped(fetch::network::TCPClient::handle_type connection_handle)
  {
    auto item = existing_subscriptions.begin();
    while(item!=existing_subscriptions.end())
    {
      if (std::get<0>(item -> first) == connection_handle)
      {
        auto handle = item -> second;
        RemoveSubscription(handle);
        item = existing_subscriptions.erase(item);
      }
      else
      {
        ++item;
      }
    }
  }

  bool RemoveSubscription(subscription_handle_type handle)
  {
    lock_type lock(mutex);
    auto location = subscriptions.find(handle);
    if (location != subscriptions.end())
    {
      subscriptions.erase(location);
      return true;
    }
    return false;
  }

private:
  class Subscription
  {
  public:
    Subscription(client_ptr client, fetch::service::subscription_handler_type handle) :
      client_(client),
      handle_(handle)
      {
      }
    virtual ~Subscription()
    {
      client_ -> Unsubscribe( handle_ );
    }
  private:
    client_ptr client_;
    fetch::service::subscription_handler_type handle_;
  };

  template<class FUNC_CLASS>
  std::shared_ptr<Subscription> CreateSubscription(client_ptr client,
                                                   protocol_handler_type protocol_number,
                                                   feed_handler_type verb,
                                                   FUNC_CLASS *func
                                                   )
  {
    auto handle = client -> Subscribe(protocol_number, verb, func);
    return std::make_shared<Subscription>(client, handle);
  }

  using subscriptions_type = std::map<uint64_t, std::shared_ptr<Subscription>>;
  subscription_handle_type handle_counter = 0;
  mutex_type mutex;
  existing_subscriptions_type existing_subscriptions;
  subscriptions_type subscriptions;
};

}
}

#endif //SUBSCRIPTIONS_CONTAINER_HPP
