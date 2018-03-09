#ifndef NODE_TO_AEA_PROTOCOL_HPP
#define NODE_TO_AEA_PROTOCOL_HPP

namespace fetch
{
namespace node_to_aea_protocol
{

class NodeToAEAProtocol : public fetch::service::Protocol {
public:

  NodeToAEAProtocol() : Protocol() {

    this->Expose(NodeToAEAProtocolFn::PING, new service::CallableClassMember<NodeToAEAProtocol, void(std::string pingMessage)>(this, &NodeToAEAProtocol::Ping) );
    this->Expose(NodeToAEAProtocolFn::BUY,  new service::CallableClassMember<NodeToAEAProtocol, std::string(std::string fromPerson)>(this, &NodeToAEAProtocol::Buy));
  }

  // TODO: (`HUT`) : make these callback registering more elegant (ask Troels) TODO: (`HUT`) : s/Troells/Troels in codebase
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
