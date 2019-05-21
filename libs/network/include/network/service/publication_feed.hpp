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
#include "network/service/abstract_publication_feed.hpp"

#include <functional>
#include <vector>
namespace fetch {
namespace service {

/* Publication functionality for a single feed.
 *
 * A class can inherith this functionality to create and publish to
 * feeds that can then later be added to protocols. Consider a message
 * passing protocol. In this protocol we would like the underlying
 * functionality to provide a feed with new messages. This can be done
 * as follows:
 *
 * ```
 * enum MessageFeed {
 *   NEW_MESSAGE = 1
 * };
 *
 * class MessageManager : public HasPublicationFeed {
 * public:
 *   // ...
 *   void PushMessage(std::string const &msg) {
 *      messages_.push_back(msg);
 *      this->Publish( MessageFeed::NEW_MESSAGE, msg );
 *   }
 *   // ...
 * };
 * ```
 *
 * In the protocol definition we can now expose the feed functionality
 * by using the method <Protocol::RegisterFeed>. More concretely, the
 * implementation of a feed in the protocol is as follows:
 *
 * ```
 * class NodeProtocol : public NodeFunctionality, public
 * fetch::service::Protocol {
 * public:
 *   NodeProtocol() : NodeFunctionality(),  fetch::service::Protocol() {
 *     using namespace fetch::service;
 *
 *     // ... SERVICE exposure ...
 *
 *     this->RegisterFeed( MessageFeed::NEW_MESSAGE, this  );
 *   }
 * };
 * ```
 *
 * As default 256 feeds are supported, but this can be changed at the
 * time where the object is constructed.
 */
class HasPublicationFeed : public AbstractPublicationFeed
{
public:
  static constexpr char const *LOGGING_NAME = "PublicationFeed";

  /* Constructor for HasPublicationFeed.
   * @n is the maximum number of support feeds.
   */
  HasPublicationFeed(std::size_t const &n = 256)
  {
    LOG_STACK_TRACE_POINT;

    publisher_.resize(n);
  }

  /* Creates a publish function.
   *
   * See <AbstractPublicationFeed::create_publisher> for documentation
   * related to general purpose of this function.
   */
  void create_publisher(feed_handler_type feed, function_type function) override
  {
    LOG_STACK_TRACE_POINT;

    if (publisher_[feed])
    {
      TODO_FAIL(
          "FeedEvents does not have support for multiple publishers. Please "
          "use MultiFeedEvents");
    }
    publisher_[feed] = function;
  }

  /* Publishes data to a feed.
   * @feed is the to which this is published.
   * @args is the argument list that will be published.
   *
   * A class that implements a given functionality would normally
   * subclass this class and use the this function to publish data. Data
   * feeds are separated such that one can use them in multiple
   * protocols. For instance, one could make a UDP feed for new blocks
   * while having another protocol that would publish messages over TCP.
   */
  template <typename... Args>
  void Publish(feed_handler_type feed, Args &&... args)
  {
    LOG_STACK_TRACE_POINT;

    serializer_type params;

    // TODO(issue 21): we should benchmark subscription too
    PackArgs(params, std::forward<Args>(args)...);

    FETCH_LOG_DEBUG(LOGGING_NAME, "Publishing data for feed ", feed);

    if (publisher_[feed])
    {
      publisher_[feed](params.data());
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Could not find publisher for ", feed);
      fetch::logger.StackTrace(2, false);
    }
  }

private:
  std::vector<function_type> publisher_;
};
}  // namespace service
}  // namespace fetch
