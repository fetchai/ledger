#ifndef SERVICE_PROTOCOL_HPP
#define SERVICE_PROTOCOL_HPP

#include "assert.hpp"
#include "service/types.hpp"
#include "service/callable_class_member.hpp"
#include "service/feed_subscription_manager.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "service/error_codes.hpp"
#include "mutex.hpp"
#include <map>
#include <memory>
#include<vector>


namespace fetch 
{
namespace service 
{
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
 * time (TODO).
 */
class Protocol 
{
public:
  typedef AbstractCallable callable_type;
  typedef byte_array::ConstByteArray byte_array_type;

  Protocol() 
  {
    for(std::size_t i = 0; i < 256; ++i) {
      members_[i] = nullptr;
    }
  }

  ~Protocol() 
  {
    for(std::size_t i = 0; i < 256; ++i) {
      if(members_[i] != nullptr) {
        delete members_[i];
      }
      
    }
  }
  
  
  /* Operator to access the different functions in the protocol.
   * @n is the idnex of callable in the protocol.
   *
   * The result of this operator is a <callable_type> that can be
   * invoked in accodance with the definition of an
   * <service::AbstractCallable>.
   *
   * This operator throws a <SerializableException> if the index is 
   *
   * @return a reference to the call. 
   */
  callable_type& operator[](function_handler_type const& n) 
  {
    LOG_STACK_TRACE_POINT;
    
    if((n >= 256) ||(members_[n] == nullptr) )
      throw serializers::SerializableException(
        error::MEMBER_NOT_FOUND,
        byte_array_type("Could not find member "));
    return *members_[n];
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
   * (TODO). 
   */
  template<typename C, typename R, typename ...Args>
  void Expose(function_handler_type const &n, C *instance, R (C::*function)(Args...)) 
  {
    
    callable_type *fnc = new service::CallableClassMember<C, R(Args...)>(instance, function );
    
    if(members_[n] != nullptr)
      throw serializers::SerializableException(
        error::MEMBER_EXISTS,
        byte_array_type("Member already exists: "));

    members_[n] = fnc;
  }

  template<typename C, typename R, typename ...Args>
  void ExposeWithClientArg(function_handler_type const &n, C *instance, R (C::*function)(Args...)) 
  {
    callable_type *fnc = new service::CallableClassMember<C, R(Args...), 1>(Callable::CLIENT_ID_ARG, instance, function );
    
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
   */
  void RegisterFeed(feed_handler_type const& feed, AbstractPublicationFeed* publisher) 
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
  void Subscribe(uint64_t const & client,   // TODO: Standardize client type over the code.
    feed_handler_type const & feed,
    subscription_handler_type const &id) 
  {
    LOG_STACK_TRACE_POINT;
    
    fetch::logger.Debug( "Making subscription for ", client,  " ", feed, " ", id);
    
    feeds_mutex_.lock();
    std::size_t i =0;
    for(;i < feeds_.size(); ++i) 
    {
      if(feeds_[i]->feed() == feed) break;
    }
    if( i == feeds_.size() ) 
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
  void Unsubscribe(uint64_t const & client,   // TODO: Standardize client type over the code.
    feed_handler_type const & feed,
    subscription_handler_type const &id) 
  {
    LOG_STACK_TRACE_POINT;
    
    feeds_mutex_.lock();
    std::size_t i =0;
    for(;i < feeds_.size(); ++i) 
    {
      if(feeds_[i]->feed() == feed) break;
    }
    if( i == feeds_.size() ) 
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
  std::vector< std::shared_ptr<FeedSubscriptionManager> >& feeds() { return feeds_; }
private:
  callable_type* members_[256] = {nullptr};
  std::vector< std::shared_ptr<FeedSubscriptionManager> > feeds_;
  fetch::mutex::Mutex feeds_mutex_;
};
}
}

#endif
