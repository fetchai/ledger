#pragma once

//#include"network/service/client.hpp"
#include "./protocol.hpp"
#include "network/service/publication_feed.hpp"

namespace fetch {
namespace subscribe {

class Node : public fetch::service::HasPublicationFeed
{
public:
  Node() {}
  ~Node() {}

  void SendMessage(std::string const &mess)
  {
    std::cout << "Publish " << mess << std::endl;
    this->Publish(fetch::protocols::SubscribeProto::NEW_MESSAGE, mess);
  }
};

}  // namespace subscribe
}  // namespace fetch
