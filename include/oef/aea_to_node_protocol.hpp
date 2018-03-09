#ifndef AEA_TO_NODE_PROTOCOL_HPP
#define AEA_TO_NODE_PROTOCOL_HPP

namespace fetch
{
namespace aea_to_node_protocol
{

class AEAToNodeProtocol : public fetch::service::Protocol {
public:

  AEAToNodeProtocol() : Protocol() {

    this->Expose(NodeToAEAProtocol::PING, new service::CallableClassMember<AEAToNodeProtocol, void(std::string pingMessage)>(this, &AEAToNodeProtocol::echoMessage) );
    this->Expose(NodeToAEAProtocol::BUY, new service::CallableClassMember<AEAToNodeProtocol, std::string(std::string fromPerson)>(this, &AEAToNodeProtocol::buy));
  }

	template <typename T>
	void registerCallback(T call) {
		callback_ = call;
	}

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
