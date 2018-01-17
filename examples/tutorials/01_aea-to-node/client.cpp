#include"commands.hpp"
#include"rpc/service_client.hpp"

#include<iostream>
using namespace fetch::rpc;

int main(int argc, char const **argv) {
  if(argc != 3) {
    std::cerr << "usage: ./" << argv[0] << " [name] [schema]" << std::endl;
    exit(-1);
  }
  
  ServiceClient client("localhost", 8080);
  client.Start();
  std::this_thread::sleep_for( std::chrono::milliseconds(100) );

  auto p = client.Call( FetchProtocols::AEA, AEACommands::REGISTER, argv[1], argv[2]  );
  if( bool(p) ) {
    std::cout << "Successfully added schema" << std::endl;
  } else {
    std::cout << "Schema already exists" << std::endl;    
  }

  client.Stop(); 
  
  return 0;
}
