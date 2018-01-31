#include <cstdlib>
#include <iostream>
#include"network/tcp_client.hpp"
using namespace fetch::network;

class Client : public TCPClient {
public:
  Client(std::string const &host,
    std::string const &port,
      ThreadManager *tmanager) :
    TCPClient(host, port, tmanager )
  {
  }  
  
  void PushMessage(message_type const &value) override
  {
    std::cout << value << std::endl;
  }

  void ConnectionFailed() override 
  {
    std::cerr << "Connection failed" << std::endl;
  }
  
  
private:

};

int main(int argc, char* argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }
    ThreadManager tmanager; 
    Client client(argv[1], argv[2], &tmanager);
    
    tmanager.Start();
    
    fetch::byte_array::ByteArray msg;
    msg.Resize(512);

    while (std::cin.getline(msg.char_pointer(), 512)) {
      msg.Resize( std::strlen(msg.char_pointer()) );
      client.Send(msg);
      msg.Resize(512);
    }

    tmanager.Stop();

  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;  
}

