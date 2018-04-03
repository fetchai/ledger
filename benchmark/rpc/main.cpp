#include"serializer/referenced_byte_array.hpp"
#include"serializer/stl_types.hpp"
#include"serializer/byte_array_buffer.hpp"
#include "serializer/counter.hpp"
#include "random/lfg.hpp"

#include"service/server.hpp"
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
// #define FETCH_DISABLE_COUT_LOGGING
#include<chrono>
#include<vector>
using namespace fetch::serializers;
using namespace fetch::byte_array;
using namespace fetch::service;
using namespace std::chrono;

fetch::random::LaggedFibonacciGenerator<> lfg;

ByteArray TestString;

enum {
  GET = 1,
  GET2 =2,
  SERVICE = 3
};
std::vector< ByteArray > TestData;
class Implementation {
public:
  std::vector< ByteArray > GetData() {
    return TestData;
  }
  
  ByteArray GetData2() {
    return TestString;
  }  
};



class ServiceProtocol : public Implementation, public Protocol {
public:
  
  ServiceProtocol() : Implementation(), Protocol() {
    
    this->Expose(GET, new CallableClassMember<Implementation, std::vector< ByteArray >() > (this, &Implementation::GetData) );
    this->Expose(GET2, new CallableClassMember<Implementation, ByteArray() > (this, &Implementation::GetData2) );    

  }  
};


// And finanly we build the service
class MyCoolService : public ServiceServer< fetch::network::TCPServer > {
public:
  MyCoolService(uint16_t port, fetch::network::ThreadManager *tm) : ServiceServer(port, tm) {
    this->Add(SERVICE, new ServiceProtocol() );
  }
};




int main() 
{
  fetch::network::ThreadManager tm;  
  MyCoolService serv(8080, &tm);
  tm.Start();
  
  while(true) {
    std::this_thread::sleep_for(std::chrono::milliseconds( 5000 ) );
    fetch::logger.StackTrace();
    
  }

  tm.Stop();
  
  return 0;
}
