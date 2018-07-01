//#include"service_consts.hpp"
#include<iostream>
#include"core/serializers/referenced_byte_array.hpp"
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"network/tcp/connection_register.hpp"
enum {
  SLOWFUNCTION = 1,
  ADD,
  GREET
};

enum {
  MYPROTO = 1
};
using namespace fetch::service;
using namespace fetch::byte_array;

int main() {
//  fetch::logger.DisableLogger();
  
  // Client setup
  fetch::network::ThreadManager tm(2);
  typedef ServiceClient< fetch::network::TCPClient > client_type;
  fetch::network::ConnectionRegister creg;
  


  tm.Start();

  {
    
    std::shared_ptr< client_type > client = creg.CreateClient<client_type>("localhost", 8080, tm);
    std::this_thread::sleep_for( std::chrono::milliseconds(100) );
    std::cout << "Was here?" << client.use_count() << std::endl;
    
  }
  std::cout << "Was here??" << std::endl;  
  std::shared_ptr< client_type > client = creg.CreateClient<client_type>("localhost", 8080, tm);
  std::this_thread::sleep_for( std::chrono::milliseconds(100) );  
    
  std::cout << client->Call( MYPROTO,GREET, "Fetch" ).As<std::string>( ) << std::endl;

  auto px = client->Call( MYPROTO,SLOWFUNCTION,"Greet"  );

  // Promises
  auto p1 = client->Call( MYPROTO,SLOWFUNCTION, 2, 7 );
  auto p2 = client->Call( MYPROTO,SLOWFUNCTION, 4, 3 );


  tm.Stop();

  return 0;

}


