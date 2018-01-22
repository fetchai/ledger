#ifndef RPC_ABSTRACT_PUBLICATION_FEED_HPP
#define RPC_ABSTRACT_PUBLICATION_FEED_HPP
#include"rpc/abstract_callable.hpp"
#include"assert.hpp"

#include<functional>

namespace fetch {
namespace rpc {
/* Super class for publishers.
 * This class is the super class abstraction for the publisher
 * classes. It defines the core functionality for integrating with the
 * rest of the service framework. A subclass of this class can then
 * define how to manage feed registrations and how to publish for a
 * given feed.
 */
class AbstractPublicationFeed {
public:
  /**
     The function signature used for 
     The reason to use std::function here instead of function pointers
     is to ensure support for lambda functions with capture and
     subsequently member functions from classes with (to this
     implementation) unknown base class.
   **/
  typedef std::function< void(fetch::byte_array::ReferencedByteArray) > function_type;

  virtual ~AbstractPublicationFeed() {}
  
  /**
     @brief creates publication function.

     This method can be invoked when defining the protocol using either
     lambda or free functions. 

     @param feed is the feed handler.
     @param function a void function that takes a byte array argument.
   **/
  virtual void create_publisher(feed_handler_type feed, function_type function) = 0;

  /**
     @brief creates publication function.

     This method can be invoked when defining the protocol to attach
     member functions as publisher.
     
     @param feed is the feed handler.
     @param function a void function that takes a byte array argument.
   **/  
  template< typename C >
  void create_publisher(feed_handler_type feed, C* cls, void (C::*function)( fetch::byte_array::ReferencedByteArray const& )  ) {
    this->create_publisher(feed, [=](fetch::byte_array::ReferencedByteArray const& msg) ->void {
        (cls->*function)(msg);
      });
  }
  
};

};
};
#endif
