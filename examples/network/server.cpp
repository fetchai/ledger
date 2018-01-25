#include <cstdlib>
#include <iostream>
#include"network/tcp_server.hpp"
using namespace fetch::network;

int main(int argc, char* argv[]) {
  try {

    if (argc != 2) {
      std::cerr << "Usage: rpc_server <port>\n";
      return 1;
    }

    
    TCPServer s(std::atoi(argv[1]));
    s.Start();
    
    while(true) {
      std::this_thread::sleep_for( std::chrono::milliseconds(100) );
    }
    s.Stop();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

