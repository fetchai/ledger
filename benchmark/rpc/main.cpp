#define FETCH_DISABLE_COUT_LOGGING
#include"serializer/referenced_byte_array.hpp"
#include"serializer/stl_types.hpp"
#include"serializer/byte_array_buffer.hpp"
#include "serializer/counter.hpp"
#include "random/lfg.hpp"

#include"service/server.hpp"
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"

#include<chrono>
#include<vector>
using namespace fetch::serializers;
using namespace fetch::byte_array;
using namespace fetch::service;
using namespace std::chrono;

fetch::random::LaggedFibonacciGenerator<> lfg;
template< typename T, std::size_t N = 256 >
void MakeString(T &str) {
  ByteArray entry;
  entry.Resize(256);
  
  for(std::size_t j=0; j < 256; ++j) {
    entry[j] = (lfg()  >> 19);      
  }
  
  str = entry;
}

template< typename T, std::size_t N = 256 >
void MakeStringVector(std::vector< T >  &vec, std::size_t size) {

  for(std::size_t i=0; i < size; ++i ){
    T s;
    MakeString(s);
    vec.push_back( s );
  }
}


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


void StartClient() 
{
  fetch::network::ThreadManager tm;  
  ServiceClient< fetch::network::TCPClient > client("localhost", 8080, &tm);
  tm.Start();

  while(true) {
    std::this_thread::sleep_for( std::chrono::milliseconds( 2000 ) );
    std::cout << "Calling ..." << std::flush;

    auto p1 = client.Call( SERVICE, GET );
    high_resolution_clock::time_point t0 = high_resolution_clock::now();        
    p1.Wait();
    std::vector< ByteArray > data;
    p1.As(data);
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    duration<double> ts1 = duration_cast<duration<double>>(t1 - t0);    
    std::cout << " DONE: " << (data.size() ) << std::endl;
    if(data.size() > 0 ) {
      std::cout <<  (data.size() * data[0].size() * 1e-6 / ts1.count() )  << " MB/s, "  << ts1.count() << " s" << std::endl;
    }
    
    
  }

  tm.Stop();
}


int main() 
{
  MakeStringVector(TestData, 100000);
  
  fetch::network::ThreadManager tm(8);  
  MyCoolService serv(8080, &tm);
  tm.Start();
  
  StartClient();
  
  tm.Stop();
  
  return 0;
}
