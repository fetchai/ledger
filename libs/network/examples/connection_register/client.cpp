//#include"service_consts.hpp"
#include<iostream>
#include"core/serializers/referenced_byte_array.hpp"
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"network/tcp/connection_register.hpp"
#include"service_consts.hpp"
#include"node_details.hpp"
using namespace fetch::service;
using namespace fetch::byte_array;

int main() {
  // Client setup
  fetch::network::ThreadManager tm(2);
  typedef ServiceClient< fetch::network::TCPClient > client_type;
  fetch::network::ConnectionRegister<fetch::NodeDetails> creg;
  
  tm.Start();

  {    
    std::shared_ptr< client_type > client = creg.CreateClient<client_type>("localhost", 8080, tm);
    std::this_thread::sleep_for( std::chrono::milliseconds(100) );
  }
  std::shared_ptr< client_type > client = creg.CreateClient<client_type>("localhost", 8080, tm);
  std::this_thread::sleep_for( std::chrono::milliseconds(100) );  
    
  std::cout << client->Call( TEST,GREET, "Fetch" ).As<std::string>( ) << std::endl;

  auto px = client->Call( TEST, ADD, int(2), int(3)  );

  // Promises
//  auto p1 = client->Call( MYPROTO,SLOWFUNCTION, 2, 7 );
//  auto p2 = client->Call( MYPROTO,SLOWFUNCTION, 4, 3 );


  tm.Stop();

  return 0;

}


