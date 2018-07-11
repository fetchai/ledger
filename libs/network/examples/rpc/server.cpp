#include"service_consts.hpp"
#include<iostream>
#include"network/service/server.hpp"
using namespace fetch::service;
using namespace fetch::byte_array;




// First we make a service implementation
class Implementation {
public:
  int SlowFunction(int const &a, int const &b) {
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
class ServiceProtocol :  public Protocol {
public:
  
  ServiceProtocol() : Protocol() {
    
    this->Expose(SLOWFUNCTION, &impl_, &Implementation::SlowFunction );
    this->Expose(ADD, &impl_, &Implementation::Add);
    this->Expose(GREET, &impl_, &Implementation::Greet);

  }
private:
  Implementation impl_;  
};


// And finanly we build the service
class MyCoolService : public ServiceServer< fetch::network::TCPServer > {
public:
  MyCoolService(uint16_t port, fetch::network::NetworkManager tm) : ServiceServer(port, tm) {
    this->Add(MYPROTO, new ServiceProtocol() );
  }
};


int main() {
  fetch::network::NetworkManager tm(8);  
  MyCoolService serv(8080, tm);
  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;                                       
  std::cin >> dummy;
  
  tm.Stop();

  return 0;

}
