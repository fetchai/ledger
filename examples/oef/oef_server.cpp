#include"oef/http_oef.hpp"
#include"oef/node_oef.hpp"

using namespace fetch::service;

// Build the protocol for OEF(rpc) and http interface
class ServiceProtocol : public HttpOEF, public fetch::service::Protocol {
public:

  ServiceProtocol(std::shared_ptr<NodeOEF> node) : Protocol(), HttpOEF(node) {

    // Expose the RPC interface to the OEF, note the HttpOEF also has a pointer to the OEF
    this->Expose(REGISTERINSTANCE,      new CallableClassMember<NodeOEF, std::string(std::string agentName, Instance)>(node.get(), &NodeOEF::RegisterInstance) );
    this->Expose(QUERY,                 new CallableClassMember<NodeOEF, std::vector<std::string>(QueryModel query)>  (node.get(), &NodeOEF::Query) );
  }
};

class ServiceOEF : public ServiceServer< fetch::network::TCPServer >, public HTTPServer  {
public:
  ServiceOEF(uint16_t port, fetch::network::ThreadManager *tm) :
    ServiceServer(port, tm),
    HTTPServer(8080, tm) {

    std::shared_ptr<NodeOEF> node = std::make_shared<NodeOEF>();
    ServiceProtocol *prot         = new ServiceProtocol(node);

    this->Add(MYPROTO, prot );

    this->AddMiddleware( fetch::http::middleware::AllowOrigin("*") );
    this->AddMiddleware( fetch::http::middleware::ColorLog);
    this->AddModule(*prot);
  }
};

int main() {
  fetch::network::ThreadManager tm(8);
  ServiceOEF serv(8090, &tm); // Note, the rpc interface is on port 8090, the http interface is 8080

  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
