#pragma once

namespace fetch {
namespace service {
  class Protocol;
  class FeedSubscriptionManager;
}}

#include "core/mutex.hpp"
#include "network/service/abstract_publication_feed.hpp"
#include "network/service/message_types.hpp"
#include "network/message.hpp"
#include "network/service/types.hpp"
#include "network/generics/work_items_queue.hpp"
#include "network/details/thread_pool.hpp"
//#include "network/service/server_interface.hpp"

#include <iterator>
#include <vector>
#include <condition_variable>

namespace fetch {
namespace service {
/* This is a subscription manager that is used on the server side.
 *
 * This class manages the client subscriptions. It is added to the
 * protocol and used by the service unit.
 *
 * A limitation of this implementation is that it does not have
 * multi-service support yet. This is, however, easily implmented at a
 * later point.
 */

class ServiceServerInterface;

class FeedSubscriptionManager
{
public:

  using mutex_type = fetch::mutex::Mutex;
  using lock_type = std::lock_guard<fetch::mutex::Mutex>;
  using service_type = fetch::service::ServiceServerInterface;
  using connection_handle_type     = uint64_t;
  using publishing_workload_type = std::tuple<service_type*, connection_handle_type, network::message_type const>;

  /* A feed that services can subscribe to.
   * @feed is the feed number defined in the protocol.
   * @publisher is an implementation class that subclasses
   * <AbstractPublicationFeed>.
   *
   * The subscription manager takes a publisher and manages its
   * subscribers. It when a protocol is added to the service, the feed
   * manager is bridges to the service via the
   * <AttachToService> function. The service
   * must implement a Send function that fulfills the concept given for
   * a service.
   */
  FeedSubscriptionManager(feed_handler_type const &feed, AbstractPublicationFeed *publisher)
    : subscribe_mutex_(__LINE__, __FILE__), feed_(feed), publisher_(publisher)
  {
    workers_ = network::MakeThreadPool(20);
    workers_ -> Start(this, &FeedSubscriptionManager::PublishToAllWorker);

  }

  /* Attaches a feed to a given service.
   * @T is the type of the service
   * @service is a pointer to the service
   *
   * This function attaches a service to the feed. It ensures that
   * messages published by the publisher are packed and send to the
   * right client.
   */
  void AttachToService(ServiceServerInterface *service);

  void PublishAll(std::vector<publishing_workload_type> &workload)
  {
    publishing_workload_.Add(workload.begin(), workload.end());
    workload.clear();
  }

  void PublishToAllWorker();

  /* Subscribe client to feed.
   * @client is the client id.
   * @id is the subscription id allocated on the client side.
   *
   * This function is intended to be used by the <Protocol> through
   * which services can subscribe their clients to the feed.
   */
  void Subscribe(uint64_t const &client, subscription_handler_type const &id)
  {
    LOG_STACK_TRACE_POINT;

    // TODO: change uint64_t to global client id.
    subscribe_mutex_.lock();
    subscribers_.push_back({client, id});
    subscribe_mutex_.unlock();
  }

  /* Unsubscribe client to feed.
   * @client is the client id.
   * @id is the subscription id allocated on the client side.
   *
   * This function is intended to be used by the <Protocol> through
   * which services can unsubscribe their clients to the feed.
   */
  void Unsubscribe(uint64_t const &client, subscription_handler_type const &id)
  {
    LOG_STACK_TRACE_POINT;

    // TODO: change uint64_t to global client id.
    subscribe_mutex_.lock();
    std::vector<std::size_t> ids;
    for (std::size_t i = 0; i < subscribers_.size(); ++i)
      if ((subscribers_[i].client == client) && (subscribers_[i].id == id)) ids.push_back(i);

    std::reverse(ids.begin(), ids.end());
    for (auto &i : ids) subscribers_.erase(std::next(subscribers_.begin(), int64_t(i)));
    subscribe_mutex_.unlock();
  }

  /* Returns the feed type.
   *
   * @return the feed type.
   */
  feed_handler_type const &feed() const { return feed_; }

  /* Returns a pointer to the abstract publisher.
   *
   * @return the publisher.
   */
  fetch::service::AbstractPublicationFeed *publisher() const { return publisher_; }

private:
  struct ClientSubscription
  {
    uint64_t                  client;
    subscription_handler_type id;
  };

  std::vector<ClientSubscription> subscribers_;
  fetch::mutex::Mutex             subscribe_mutex_;
  feed_handler_type               feed_;

  fetch::service::AbstractPublicationFeed *publisher_ = nullptr;

  generics::WorkItemsQueue<publishing_workload_type> publishing_workload_;
  network::ThreadPool workers_;
};
}  // namespace service
}  // namespace fetch
