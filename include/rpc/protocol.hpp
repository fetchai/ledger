#ifndef RPC_PROTOCOL_HPP
#define RPC_PROTOCOL_HPP

#include "assert.hpp"
#include "rpc/types.hpp"
#include "rpc/callable_class_member.hpp"
#include "rpc/feed.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "rpc/error_codes.hpp"
#include "mutex.hpp"
#include <map>
#include <memory>
#include<vector>

namespace fetch {
namespace rpc {

class Protocol {
 public:
  typedef AbstractCallable callable_type;
  typedef byte_array::ReferencedByteArray byte_array_type;

  callable_type& operator[](function_handler_type const& n) {
    if(members_[n] == nullptr)
      throw serializers::SerializableException(
          error::MEMBER_NOT_FOUND,
          byte_array_type("Could not find member "));
    return *members_[n];
  }

  void Expose(function_handler_type const& n, callable_type* fnc) {
    if(members_[n] != nullptr)
      throw serializers::SerializableException(
          error::MEMBER_EXISTS,
          byte_array_type("Member already exists: "));

    members_[n] = fnc;
  }


  template< typename T > 
  void RegisterFeed(feed_handler_type const& feed, T* publisher) {
    feeds_.push_back(std::make_shared<Feed>(feed, publisher));
  }

  // TODO: Standardize client type over the code.
  void Subscribe(uint64_t const & client,
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

  // TODO: Standardize client type over the code.
  void Unsubscribe(uint64_t const & client,
                 feed_handler_type const & feed,
                 subscription_handler_type const &id) {
    std::cout << "Unsubscribing " << client << " " << feed << " " << id << std::endl;
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
  
  std::vector< std::shared_ptr<Feed> >& feeds() { return feeds_; }
 private:
  callable_type* members_[256] = {nullptr};
  std::vector< std::shared_ptr<Feed> > feeds_;
  fetch::mutex::Mutex feeds_mutex_;
};
};
};

#endif
