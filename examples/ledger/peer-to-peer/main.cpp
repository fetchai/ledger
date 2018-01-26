
#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols/node_discovery.hpp"
#include<vector>
#include<memory>

using namespace fetch::protocols;
enum FetchProtocols {
  DISCOVERY = 1
};

class FetchService : public fetch::service::ServiceServer< fetch::network::TCPServer > {
public:
  FetchService(uint16_t port, std::string const&info) : ServiceServer(port) {
    discovery_ =  new DiscoveryProtocol();
    
    this->Add(FetchProtocols::DISCOVERY, discovery_ );
  }

  ~FetchService() {
    delete discovery_;
  }

  void Bootstrap(std::string const &ip, uint16_t const &port) {
    
  }

  void ConnectToPeer( ) {
    auto handle1 = client.Subscribe(FetchProtocols::PEER_TO_PEER, PeerToPeerFeed::NEW_MESSAGE, new Function< void(std::string) >([](std::string const&msg) {
          std::cout << VT100::GetColor("blue", "default") << "Got message: " << msg << VT100::DefaultAttributes() <<  std::endl;
        }) );
    
    client.Subscribe(FetchProtocols::PEER_TO_PEER, PeerToPeerFeed::NEW_MESSAGE, new Function< void(std::string) >([](std::string const&msg) {
          std::cout << VT100::GetColor("red", "default") << "Got message 2: " << msg << VT100::DefaultAttributes() <<  std::endl;
        }) );    

  }
private:
  DiscoveryProtocol *discovery_ = nullptr;
  //  SubscriptionManager subscriptions_;

};


int main() {
  FetchService service(1337);
  
  return 0;
}
