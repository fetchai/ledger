#ifndef NODE_HPP
#define NODE_HPP
#include"remote_protocol.hpp"

class FetchService : public fetch::rpc::ServiceServer {
public:
  FetchService(uint16_t port, std::string const&info) : ServiceServer(port) {
    remote_ =  new RemoteProtocol(info);
    this->Add(FetchProtocols::REMOTE_CONTROL, remote_ );
  }
private:
  RemoteProtocol *remote_;
};

#endif
