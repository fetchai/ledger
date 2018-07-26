#pragma once
#include "core/mutex.hpp"
#include "network/service/abstract_publication_feed.hpp"
#include "network/service/message_types.hpp"
#include "network/service/types.hpp"

#include <iterator>
#include <vector>

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
class FeedSubscriptionManager
{
public:
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
  FeedSubscriptionManager(feed_handler_type const &feed,
                          AbstractPublicationFeed *publisher)
      : subscribe_mutex_(__LINE__, __FILE__), feed_(feed), publisher_(publisher)
  {}

  /* Attaches a feed to a given service.
   * @T is the type of the service
   * @service is a pointer to the service
   *
   * This function attaches a service to the feed. It ensures that
   * messages published by the publisher are packed and send to the
   * right client.
   */
  template <typename T>
  void AttachToService(T *service)
  {
    LOG_STACK_TRACE_POINT;

    publisher_->create_publisher(
        feed_, [=](fetch::byte_array::ConstByteArray const &msg) {
          serializer_type params;
          params << SERVICE_FEED << feed_;
          uint64_t p = params.Tell();
          params << subscription_handler_type(0);  // placeholder

          params.Allocate(msg.size());
          params.WriteBytes(msg.pointer(), msg.size());

          subscribe_mutex_.lock();
          std::vector<std::size_t> remove;
          for (std::size_t i = 0; i < subscribers_.size(); ++i)
          {
            auto &s = subscribers_[i];
            params.Seek(p);
            params << s.id;

            // Copy is important here as we reuse an existing buffer
            if (!service->DeliverResponse(s.client, params.data().Copy()))
            {
              remove.push_back(i);
            }
          }

          std::reverse(remove.begin(), remove.end());
          for (auto &i : remove)
            subscribers_.erase(std::next(subscribers_.begin(), int64_t(i)));
          subscribe_mutex_.unlock();
        });
  }

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
      if ((subscribers_[i].client == client) && (subscribers_[i].id == id))
        ids.push_back(i);

    std::reverse(ids.begin(), ids.end());
    for (auto &i : ids)
      subscribers_.erase(std::next(subscribers_.begin(), int64_t(i)));
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
  fetch::service::AbstractPublicationFeed *publisher() const
  {
    return publisher_;
  }

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
};
}  // namespace service
}  // namespace fetch

