#define FETCH_DISABLE_LOG
#include<iostream>
#include"network/service/client.hpp"
#include"core/logger.hpp"
#include"core/commandline/cli_header.hpp"
#include"storage/indexed_document_store.hpp"
using namespace fetch::service;
using namespace fetch::byte_array;


int main() {
  fetch::commandline::DisplayCLIHeader("Single state shard client");
  
  // Client setup
  fetch::network::ThreadManager tm(2);
  ServiceClient< fetch::network::TCPClient > client("localhost", 8080, tm);

  tm.Start();

  std::string line = "";
  
  while( (std::cin) && (line !="quit") ) {
    std::cout << " >> ";    
    std::getline(std::cin, line);
    std::size_t p = line.find(' ');
    if(p > 0) {
      std::string command = line.substr(0, std::size_t(p));
      std::string params = line.substr(std::size_t(p + 1), std::size_t(line.size() - 1 - p));

      if(command == "get") {
        std::cout << "Getting " << params << std::endl;
        std::cout << "Result: " << client.Call(0, fetch::storage::DocumentStoreProtocol::GET, params).As<std::string>() << std::endl;
        
      }
      
//      std::cout <<  << std::endl;      
//      std::cout <<  << std::endl;
      
    }
    
  }
  

//  client.WithDecorators(aes, ... ).Call( MYPROTO,SLOWFUNCTION, 4, 3 );

  tm.Stop();
  
  return 0;
  
}
