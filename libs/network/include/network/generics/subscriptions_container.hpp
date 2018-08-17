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
using protocol_number_type = fetch::service::protocol_handler_type;
using feed_handler_type = fetch::service::feed_handler_type;
using subs_handle_type = uint64_t;
using client_type = fetch::service::ServiceClient;
using client_ptr = std::shared_ptr<client_type>;

using client_handle_type = uint64_t;
using verb_type = uint64_t;

using existing_subs_type = std::map<
  std::tuple<client_handle_type, verb_type, protocol_number_type>,
  subs_handle_type
>;

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
  subs_handle_type Subscribe(
    client_ptr client,
    protocol_number_type protocol_number,
    feed_handler_type verb,
    const std::string &name,
    FUNC_CLASS *func
    )
  {
    lock_type lock(mutex);
    auto identifier = std::make_tuple(client -> handle(), protocol_number, verb);

    auto search_result = existing_subs.find(identifier);

    if (search_result != existing_subs.end())
    {
      return search_result -> second;
    }

    auto r = handle_counter++;
    existing_subs[identifier] = r;
    subs[r] = CreateSubscription(client, protocol_number, verb, name, func);
    return r;
  }

  void ConnectionDropped(fetch::network::TCPClient::handle_type connection_handle)
  {
    lock_type lock(mutex);
    auto item = existing_subs.begin();
    while(item!=existing_subs.end())
    {
      if (std::get<0>(item -> first) == connection_handle)
      {
        auto handle = item -> second;
        RemoveSubscription(handle);
        item = existing_subs.erase(item);
      }
      else
      {
        ++item;
      }
    }
  }

  void AssociateName(const std::string &name, client_handle_type connection_handle,
                     protocol_number_type proto=0,
                     feed_handler_type verb=0)
  {
    lock_type lock(mutex);
    for(auto &item : existing_subs)
    {
      if (
          (std::get<0>(item.first) == connection_handle)
          &&
          (!proto || std::get<2>(item.first) == proto)
          &&
          (!verb || std::get<1>(item.first) == verb)
          )
      {
        subs[item.second] -> name_ = name;
      }
    }
  }

  void VisitSubscriptions(std::function<void (client_ptr)> func)
  {
    std::list<std::shared_ptr<Subscription>> subscriptions_local;

    {
      lock_type lock(mutex);
      for(auto &subscription : subs)
      {
        subscriptions_local.push_back(subscription.second);
      }
    }

    for(auto subscription : subscriptions_local)
    {
      func(subscription->GetClient());
    }
  }

  std::vector<std::string> GetAllSubscriptions(protocol_number_type proto, feed_handler_type verb)
  {
    lock_type lock(mutex);
    std::vector<std::string> r;
    for(auto &item : existing_subs)
    {
      if (
          (!proto || std::get<1>(item.first) == proto)
          &&
          (!verb || std::get<2>(item.first) == verb)
          )
      {
        r.push_back(subs[item.second] -> getName());
      }
    }
    return r;
  }

  bool RemoveSubscription(subs_handle_type handle)
  {
    lock_type lock(mutex);
    auto location = subs.find(handle);
    if (location != subs.end())
    {
      subs.erase(location);
      return true;
    }
    return false;
  }

private:
  class Subscription
  {
  public:
    friend class SubscriptionsContainer;
    Subscription(client_ptr client,
                 fetch::service::subscription_handler_type handle,
                 const std::string &name)
      : client_(client)
      , handle_(handle)
      , name_(name)
      {
      }
    virtual ~Subscription()
    {
      client_ -> Unsubscribe( handle_ );
    }
    const std::string &getName() { return name_; }

    client_ptr GetClient() { return client_; }
  private:
    client_ptr client_;
    fetch::service::subscription_handler_type handle_;
    std::string name_;
  };

  template<class FUNC_CLASS>
  std::shared_ptr<Subscription> CreateSubscription(client_ptr client,
                                                   protocol_number_type protocol_number,
                                                   feed_handler_type verb,
                                                   const std::string &name,
                                                   FUNC_CLASS *func
                                                   )
  {
    auto handle = client -> Subscribe(protocol_number, verb, func);
    return std::make_shared<Subscription>(client, handle, name);
  }

  using subs_type = std::map<uint64_t, std::shared_ptr<Subscription>>;
  subs_handle_type handle_counter = 0;
  mutex_type mutex;
  existing_subs_type existing_subs;
  subs_type subs;
};

}
}

#endif //SUBSCRIPTIONS_CONTAINER_HPP
