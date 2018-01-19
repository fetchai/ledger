#ifndef RPC_ABSTRACT_PUBLICATION_FEED_HPP
#define RPC_ABSTRACT_PUBLICATION_FEED_HPP
#include"rpc/abstract_callable.hpp"
#include"assert.hpp"

#include<functional>

namespace fetch {
namespace rpc {

class AbstractPublicationFeed {
public:
  /**
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
     
     @param a void function that takes a byte array argument.
   **/
  virtual void create_publisher(feed_handler_type feed, function_type function) = 0;

  /**
     @brief creates publication function.

     This method can be invoked when defining the protocol to attach
     member functions as publisher.
     
     @param a void function that takes a byte array argument.
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
