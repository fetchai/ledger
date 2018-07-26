#pragma once

#include"./commands.hpp"

namespace fetch
{
namespace protocols
{

/* Class defining a particular protocol, that is, the calls that are
 * exposed to remotes.
 */
template<typename T>
class QuickStartProtocol : public fetch::service::Protocol
{
public:

  /* Constructor for quick start protocol attaches the functions to the protocol enums
   *
   * @node is an instance of the object we want to expose the interface of
   */
  QuickStartProtocol(std::shared_ptr<T> node) : Protocol()
  {
    // Here we expose the functions in our class (ping, receiveMessage etc.) using our protocol enums
    this->Expose(QuickStart::PING,         node.get(), &T::ping);
    this->Expose(QuickStart::SEND_MESSAGE, node.get(), &T::receiveMessage);
    this->Expose(QuickStart::SEND_DATA,    node.get(), &T::receiveData);
  }
};

}
}

