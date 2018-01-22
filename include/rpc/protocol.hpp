#ifndef RPC_PROTOCOL_HPP
#define RPC_PROTOCOL_HPP

#include "assert.hpp"
#include "rpc/types.hpp"
#include "rpc/callable_class_member.hpp"
#include "rpc/feed_subscription_manager.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "rpc/error_codes.hpp"
#include "mutex.hpp"
#include <map>
#include <memory>
#include<vector>

namespace fetch {
namespace rpc {
/* A class that defines a generic protocol.
 *
 * This class is used for defining a general protocol with
 * remote-function-calls (RPCs) and data feeds. The RPCs are defined
 * from a C++ function signature using any sub class of
 * <rpc::AbstractCallable> including <rpc::Function> and
 * <rpc::CallableClassMember>. The feeds are available from any
 * functionality implementation that sub-classes
 * <rpc::HasPublicationFeed>.
 *
 * A current limitation of the implementation is that there is only
 * support for 256 RPC functions. It the next version of this class,
 * this should be changed to be variable and allocated at construction
 * time (TODO).
 */
class Protocol {
 public:
  typedef AbstractCallable callable_type;
  typedef byte_array::ReferencedByteArray byte_array_type;

  /* Operator to access the different functions in the protocol.
   * @n is the idnex of callable in the protocol.
   *
   * The result of this operator is a <callable_type> that can be
   * invoked in accodance with the definition of an
   * <rpc::AbstractCallable>.
   *
   * This operator throws a <SerializableException> if the index is 
   *
   * @return a reference to the call. 
   */
  callable_type& operator[](function_handler_type const& n) {
    if((n >= 256) ||(members_[n] == nullptr) )
      throw serializers::SerializableException(
          error::MEMBER_NOT_FOUND,
          byte_array_type("Could not find member "));
    return *members_[n];
  }

  /* Exposes a function or class member function.
   * @n is a unique identifier for the callable being exposed
   * @fnc is a pointer to the callable function.
   * 
   * The pointer provided is used to invoke the callable when a call
   * matching the identifier is recieved by a service.
   *
   * In the next implementation of this, one should use unique_ptr
   * rather than a raw pointer. This will have no impact on the rest of
   * the code as it is always a reference return and not the raw pointer
   * (TODO). 
   */
  void Expose(function_handler_type const& n, callable_type* fnc) {
    if(members_[n] != nullptr)
      throw serializers::SerializableException(
          error::MEMBER_EXISTS,
          byte_array_type("Member already exists: "));

    members_[n] = fnc;
  }

  /* Registers a feed from an implementation.
   * @feed is the unique feed identifier.
   * @publisher is a class that subclasses <AbstractPublicationFeed>.
   *
   *  
   */
  void RegisterFeed(feed_handler_type const& feed, AbstractPublicationFeed* publisher) {
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
  void Subscribe(uint64_t const & client,   // TODO: Standardize client type over the code.
                 feed_handler_type const & feed,
                 subscription_handler_type const &id) {
    std::cout << "Making subscription for " << client << " " << feed << " " << id << std::endl;
    feeds_mutex_.lock();
    std::size_t i =0;
    for(;i < feeds_.size(); ++i) {
      if(feeds_[i]->feed() == feed) break;
    }
    if( i == feeds_.size() ) {
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
  void Unsubscribe(uint64_t const & client,   // TODO: Standardize client type over the code.
                 feed_handler_type const & feed,
                 subscription_handler_type const &id) {
    feeds_mutex_.lock();
    std::size_t i =0;
    for(;i < feeds_.size(); ++i) {
      if(feeds_[i]->feed() == feed) break;
    }
    if( i == feeds_.size() ) {
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
  std::vector< std::shared_ptr<FeedSubscriptionManager> >& feeds() { return feeds_; }
 private:
  callable_type* members_[256] = {nullptr};
  std::vector< std::shared_ptr<FeedSubscriptionManager> > feeds_;
  fetch::mutex::Mutex feeds_mutex_;
};
};
};

#endif
