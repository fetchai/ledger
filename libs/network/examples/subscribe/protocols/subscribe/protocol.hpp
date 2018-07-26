#ifndef PROTOCOLS_SUBSCRIBE_PROTOCOL_HPP
#define PROTOCOLS_SUBSCRIBE_PROTOCOL_HPP

#include "./commands.hpp"
#include "./node.hpp"

namespace fetch {
namespace protocols {

/* Class defining a particular protocol, that is, the calls that are
 * exposed to remotes. In this instance we are allowing interested parties to
 * register to listen to 'new message's from the Node
 */
class SubscribeProtocol : public fetch::service::Protocol,
                          public subscribe::Node
{
public:
  /*
   * Constructor for subscribe protocol attaches the functions to the protocol
   * enums
   */
  SubscribeProtocol() : Protocol(), Node()
  {
    // We are allowing 'this' Node to register the feed new_message on the
    // protocol
    this->RegisterFeed(SubscribeProto::NEW_MESSAGE, this);
  }
};

}  // namespace protocols
}  // namespace fetch

#endif
