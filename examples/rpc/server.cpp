#include<iostream>
#include"rpc/service_server.hpp"
using namespace fetch::rpc;
using namespace fetch::byte_array;




// First we make a service implementation
class Implementation {
public:
  int SlowFunction(int a, int b) {
    std::this_thread::sleep_for( std::chrono::milliseconds(20) );
    return a + b;
  }
  
  int Add(int a, int b) {
    return a + b;
  }
  
  std::string Greet(std::string name) {
    return "Hello, " + name;
  }      
};




// Next we make a protocol for the implementation
class RPCProto : public Implementation, public Protocol {
public:
  RPCProto() : Implementation(), Protocol() {
    this->Expose("SlowFunction", new CallableClassMember<Implementation, int(int, int)>(this, &Implementation::SlowFunction) );
    this->Expose("Add", new CallableClassMember<Implementation, int(int, int)>(this, &Implementation::Add) );
    this->Expose("Greet", new CallableClassMember<Implementation, std::string(std::string)>(this, &Implementation::Greet) );
  }  
};


// And finanly we build the service
class MyCoolService : public ServiceServer {
public:
  MyCoolService(uint16_t port) : ServiceServer(port) {
    this->Add("Mod", new RPCProto() );
  }
};


int main() {
  MyCoolService serv(8080);
  serv.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;                                       
  std::cin >> dummy;
  
  serv.Stop();

  return 0;

}
