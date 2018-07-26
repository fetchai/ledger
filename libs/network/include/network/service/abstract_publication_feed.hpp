#pragma once
#include "core/assert.hpp"
#include "network/service/abstract_callable.hpp"

#include <functional>

namespace fetch {
namespace service {
/* Super class for publishers.
 * This class is the super class abstraction for the publisher
 * classes. It defines the core functionality for integrating with the
 * rest of the service framework. A subclass of this class can then
 * define how to manage feed registrations and how to publish for a
 * given feed.
 */
class AbstractPublicationFeed
{
public:
  /* The function signature used for
   * The reason to use std::function here instead of function pointers
   * is to ensure support for lambda functions with capture and
   * subsequently member functions from classes with (to this
   * implementation) unknown base class.
   */
  typedef std::function<void(fetch::byte_array::ConstByteArray)> function_type;

  virtual ~AbstractPublicationFeed() {}

  /* Creates publication function.
   * @feed is the feed handler.
   * @function a void function that takes a byte array argument.
   *
   * This method can be invoked when defining the protocol using either
   * lambda or free functions.
   **/
  virtual void create_publisher(feed_handler_type feed, function_type function) = 0;

  /* Creates publication function.
   * @feed is the feed handler.
   * @function a void function that takes a byte array argument.
   *
   * This method can be invoked when defining the protocol to attach
   * member functions as publisher.
   **/
  template <typename C>
  void create_publisher(feed_handler_type feed, C *cls,
                        void (C::*function)(fetch::byte_array::ConstByteArray const &))
  {
    LOG_STACK_TRACE_POINT;

    this->create_publisher(
        feed, [=](fetch::byte_array::ConstByteArray const &msg) -> void { (cls->*function)(msg); });
  }
};
}  // namespace service
}  // namespace fetch
