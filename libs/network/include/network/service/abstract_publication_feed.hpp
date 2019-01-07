#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

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
  using function_type = std::function<void(fetch::byte_array::ConstByteArray)>;

  virtual ~AbstractPublicationFeed()
  {}

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
