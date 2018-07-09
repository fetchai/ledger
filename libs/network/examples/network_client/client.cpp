#include <cstdlib>
#include <iostream>
#include "core/serializers/stl_types.hpp"
#include"network/tcp/tcp_client.hpp"
using namespace fetch::network;

class Client : public TCPClient {
public:
  Client(std::string const &host,
    std::string const &port,
      ThreadManager tmanager) :
    TCPClient(tmanager )
  {
    Connect(host, port);
    this->OnMessage([](message_type const &value) 
      {
        std::cout << value << std::endl;
      });
    this->OnConnectionFailed([]() 
      {
        std::cerr << "Connection failed" << std::endl;
      });
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
    tmanager.Start();

    // Attempt to break the connection
    for (std::size_t i = 0; i < 1000; ++i)
    {
      Client client(argv[1], argv[2], tmanager);

      while(!client.is_alive())
      {
        std::cout << "Waiting for client to connect" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      fetch::byte_array::ByteArray msg0("Testing rapid string pushing");
      client.Send(msg0.Copy());
    }

    Client client(argv[1], argv[2], tmanager);
    fetch::byte_array::ByteArray msg;
    msg.Resize(512);

    while (std::cin.getline(msg.char_pointer(), 512)) {
      msg.Resize( std::strlen(msg.char_pointer()) );
      client.Send(msg.Copy());
      msg.Resize(512);
    }

    tmanager.Stop();

  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;  
}

