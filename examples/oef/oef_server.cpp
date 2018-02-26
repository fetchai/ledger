#include"oef_service_consts.hpp"
#include<iostream>
#include"service/server.hpp"

//#include "old_oef_codebase/lib/include/serviceDirectory.h"
#include "oef/ServiceDirectory.h"
using namespace fetch::service;
using namespace fetch::byte_array;


// First we make a service implementation
class Implementation {

public:
  // TODO: (`HUT`) : delete these
  int SlowFunction(int a, int b) {
    std::this_thread::sleep_for( std::chrono::milliseconds(20) );
    return a + b;
  }

  // TODO: (`HUT`) : delete these
  int Add(int a, int b) {
    return a + b;
  }

  // TODO: (`HUT`) : delete these
  std::string Greet(std::string name) {
    return "Hello, " + name;
  }

  std::string RegisterDataModel(std::string agentName, Instance instance) {

    auto result = serviceDirectory_.registerAgent(instance, agentName);
    return std::to_string(result);
  }

  std::vector<std::string> Query(std::string agentName, QueryModel query) {

    return serviceDirectory_.query(query);
  }

private:
  ServiceDirectory serviceDirectory_;
};

// Next we make a protocol for the implementation
class ServiceProtocol : public Implementation, public Protocol {
public:

  ServiceProtocol() : Implementation(), Protocol() {

    this->Expose(SLOWFUNCTION,          new CallableClassMember<Implementation, int(int, int)>(this, &Implementation::SlowFunction) );                   // TODO: (`HUT`) : delete this
    this->Expose(ADD,                   new CallableClassMember<Implementation, int(int, int)>(this, &Implementation::Add) );                            // TODO: (`HUT`) : delete this
    this->Expose(GREET,                 new CallableClassMember<Implementation, std::string(std::string)>(this, &Implementation::Greet) );               // TODO: (`HUT`) : delete this
    this->Expose(REGISTERDATAMODEL,     new CallableClassMember<Implementation, std::string(std::string agentName, Instance)>(this, &Implementation::RegisterDataModel) );
    this->Expose(QUERY,                 new CallableClassMember<Implementation, std::vector<std::string>(std::string agentName, QueryModel query)>(this, &Implementation::Query) );
  }
};

// And finanly we build the service
class MyCoolService : public ServiceServer< fetch::network::TCPServer > {
public:
  MyCoolService(uint16_t port, fetch::network::ThreadManager *tm) : ServiceServer(port, tm) {
    this->Add(MYPROTO, new ServiceProtocol() );
  }
};

int main() {
  fetch::network::ThreadManager tm(8);
  MyCoolService serv(8080, &tm);
  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
