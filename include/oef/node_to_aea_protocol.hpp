#ifndef NODE_TO_AEA_PROTOCOL_HPP
#define NODE_TO_AEA_PROTOCOL_HPP

namespace fetch
{
namespace node_to_aea_protocol
{

class NodeToAEAProtocol : public fetch::service::Protocol {
public:

  NodeToAEAProtocol() : Protocol() {

    this->Expose(NodeToAEAProtocolFn::PING, new service::CallableClassMember<NodeToAEAProtocol, void(std::string pingMessage)>(this, &NodeToAEAProtocol::echoMessage) );
    this->Expose(NodeToAEAProtocolFn::BUY,  new service::CallableClassMember<NodeToAEAProtocol, std::string(std::string fromPerson)>(this, &NodeToAEAProtocol::buy));
  }

  template <typename T>
  void registerCallback(T call) {
      callback_ = call;
  }

  // TODO: (`HUT`) : make these callback registering more elegant
  void echoMessage(std::string mess) {
    if(callback_ != nullptr) {
      callback_(mess);
    }
  }

  std::string buy(std::string mess) {

    if(onBuy_ != nullptr) {
      return onBuy_(mess);
    }

    return "nothing";
  }

  std::function<std::string(std::string)> &onBuy() { return onBuy_; }

private:
  std::function<void(std::string)> callback_     = nullptr;
  std::function<std::string(std::string)> onBuy_ = nullptr;
};

}
}

#endif
