#include"oef/http_interface.h"
#include"oef/NodeOEF.h"

using namespace fetch::service; // TODO: (`HUT`) : using namespaces is discouraged (especially in .h files)

// Build our protocol for OEF(rpc) and http interface
class ServiceProtocol : public NodeOEF, public HTTPOEF, public fetch::service::Protocol {
public:

  ServiceProtocol() : NodeOEF(), Protocol(), HTTPOEF() {
    this->Expose(REGISTERDATAMODEL,     new CallableClassMember<NodeOEF, std::string(std::string agentName, Instance)>                     (this, &NodeOEF::RegisterDataModel) );
    this->Expose(QUERY,                 new CallableClassMember<NodeOEF, std::vector<std::string>(std::string agentName, QueryModel query)>(this, &NodeOEF::Query) );
  }
};

class MyCoolService : public ServiceServer< fetch::network::TCPServer >, public HTTPServer  {
public:
  MyCoolService(uint16_t port, fetch::network::ThreadManager *tm) :
    ServiceServer(port, tm),
    HTTPServer(8080, tm) {

    ServiceProtocol *prot = new ServiceProtocol();

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
