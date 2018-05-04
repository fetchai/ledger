#define FETCH_DISABLE_COUT_LOGGING
#include<iostream>
#include"serializers/referenced_byte_array.hpp"
#include"serializers/stl_types.hpp"
#include"serializers/byte_array_buffer.hpp"
#include "serializers/counter.hpp"
#include "random/lfg.hpp"

#include"service/server.hpp"
#include"serializers/referenced_byte_array.hpp"
#include"service/client.hpp"
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

// First we make a service implementation
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





void TestSerializationSpeed() 
{
  std::vector< ByteArray > a,b,c;
  for(std::size_t i=0; i < 100000; ++i ){
    ByteArray entry;
    entry.Resize(256);

    for(std::size_t j=0; j < 256; ++j) {
      entry[j] = (lfg()  >> 19);      
    }

    a.push_back(entry);
//    a.push_back("W18rRzylnPc35jkyCWkObNT0LWkU4uswXkpZkRzOQFWG5sy9Gb9ji9yP7Wzsc5NC4y5RJuQN9Og29RrPJF2ZMhwnrIL3Z4AvkHkq");    
//    if(i % 3) c.push_back(entry);    
  }
  
  
  ByteArrayBuffer buffer;


  high_resolution_clock::time_point t1 = high_resolution_clock::now();  
  std::sort(a.begin(), a.end());
  
  SizeCounter< ByteArrayBuffer > counter;
  
  counter << a;  
  buffer.Reserve( counter.size() );    
  buffer << a;
  
  high_resolution_clock::time_point t2 = high_resolution_clock::now();  
  buffer.Seek(0);
  buffer >> b;
  std::sort(c.begin(), c.end());
  high_resolution_clock::time_point t3 = high_resolution_clock::now();
  duration<double> ts1 = duration_cast<duration<double>>(t2 - t1);
  duration<double> ts2 = duration_cast<duration<double>>(t3 - t2);
  std::cout << "It took " << ts1.count() << " seconds.";
  std::cout << std::endl;    
  std::cout << "It took " << ts2.count() << " seconds.";  
  std::cout << std::endl;
  for(std::size_t i=0; i < b.size(); ++i) {
    if(a[i] != b[i]) {
      std::cerr << "Mismatch" << std::endl;
      exit(-1);
    }
  }
}


int main() {
  TestSerializationSpeed();

  std::size_t XX = 0;
  TestString.Resize( 100000 * 256 );
  for(std::size_t i=0; i < 100000; ++i ){
    ByteArray entry;
    entry.Resize(256);

    for(std::size_t j=0; j < 256; ++j) {
      entry[j] = (lfg()  >> 19);
      TestString[XX++] = (lfg()  >> 19);
    }

    TestData.push_back(entry);
  }

  fetch::network::ThreadManager tm(8);
  fetch::network::ThreadManager tm2(8);  
  MyCoolService serv(8080, &tm);
  tm.Start();

  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  
  ServiceClient< fetch::network::TCPClient > client("localhost", 8080, &tm2);
  tm2.Start();
  
  high_resolution_clock::time_point t1 = high_resolution_clock::now();  
  std::vector< ByteArray > ret;
  std::cout << "calling " << std::endl;
  client.Call( SERVICE,GET  ).As( ret );
  std::cout << "Done " << std::endl;  
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  ByteArray ret2;
  std::cout << "calling " << std::endl;
  client.Call( SERVICE,GET2  ).As( ret2 );
  std::cout << "Done " << std::endl;
  high_resolution_clock::time_point t3 = high_resolution_clock::now();  
  duration<double> ts1 = duration_cast<duration<double>>(t2 - t1);
  std::cout << "It took " << ts1.count() << " seconds." << std::endl;  
  duration<double> ts2 = duration_cast<duration<double>>(t3 - t2);
  std::cout << "It took " << ts2.count() << " seconds." << std::endl;  


  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  tm.Stop();

  return 0;
}
