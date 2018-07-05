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


  // With authentication
  {    
    std::shared_ptr< client_type > client = creg.CreateClient<client_type>("localhost", 8080, tm);
    std::this_thread::sleep_for( std::chrono::milliseconds(100) );

    client->Call( AUTH, HELLO ).Wait();

    std::cout << client->Call( TEST,GREET, "Fetch" ).As<std::string>( ) << std::endl;
  }



  // Without 
  std::shared_ptr< client_type > client = creg.CreateClient<client_type>("localhost", 8080, tm);
  std::this_thread::sleep_for( std::chrono::milliseconds(100) );  
  std::cout << client->Call( TEST,GREET, "Fetch" ).As<std::string>( ) << std::endl;
  auto px = client->Call( TEST, ADD, int(2), int(3)  );


  tm.Stop();

  return 0;

}


