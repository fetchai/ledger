#include"oef/HttpOEF.h"
#include"oef/NodeOEF.h"

using namespace fetch::service; // TODO: (`HUT`) : using namespaces is discouraged (especially in .h files)

// Build our protocol for OEF(rpc) and http interface
class ServiceProtocol : public HttpOEF, public fetch::service::Protocol {
public:

  ServiceProtocol(NodeOEF *node) : Protocol(), HttpOEF(node) {
    this->Expose(REGISTERDATAMODEL,     new CallableClassMember<NodeOEF, std::string(std::string agentName, Instance)>                     (node, &NodeOEF::RegisterDataModel) );
    this->Expose(QUERY,                 new CallableClassMember<NodeOEF, std::vector<std::string>(std::string agentName, QueryModel query)>(node, &NodeOEF::Query) );
  }
};

class MyCoolService : public ServiceServer< fetch::network::TCPServer >, public HTTPServer  {
public:
  MyCoolService(uint16_t port, fetch::network::ThreadManager *tm) :
    ServiceServer(port, tm),
    HTTPServer(8080, tm) {

    NodeOEF *node         = new NodeOEF();
    ServiceProtocol *prot = new ServiceProtocol(node);

    this->Add(MYPROTO, prot );

    this->AddMiddleware( fetch::http::middleware::AllowOrigin("*") );
    this->AddMiddleware( fetch::http::middleware::ColorLog);
    this->AddModule(*prot);
  }
};

int main() {
  fetch::network::ThreadManager tm(8);
  MyCoolService serv(8090, &tm);                                   // TODO: (`HUT`) : rename from MyCoolService

  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
