#ifndef NODE_TO_AEA_PROTOCOL_HPP
#define NODE_TO_AEA_PROTOCOL_HPP

#include"protocols/node_to_aea/commands.hpp"

namespace fetch
{
namespace protocols
{

class NodeToAEAProtocol : public fetch::service::Protocol {
public:

  NodeToAEAProtocol() : Protocol() {

    this->Expose(NodeToAEAReverseRPC::PING, new service::CallableClassMember<NodeToAEAProtocol, void(std::string pingMessage)>(this, &NodeToAEAProtocol::Ping) );
    this->Expose(NodeToAEAReverseRPC::BUY,  new service::CallableClassMember<NodeToAEAProtocol, std::string(std::string fromPerson)>(this, &NodeToAEAProtocol::Buy));
  }

  // TODO: (`HUT`) : make these callback registering more elegant (ask Troels)
  void Ping(std::string mess) {
    if(onPing_ != nullptr) {
      onPing_(mess);
    }
  }

  std::string Buy(std::string mess) {

    if(onBuy_ != nullptr) {
      return onBuy_(mess);
    }

    return "nothing";
  }

  std::function<void(std::string)>        &onPing(){ return onPing_; }
  std::function<std::string(std::string)> &onBuy() { return onBuy_; }

private:
  std::function<void(std::string)>        onPing_   = nullptr;
  std::function<std::string(std::string)> onBuy_    = nullptr;
};

}
}

#endif
