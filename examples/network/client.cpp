#include <cstdlib>
#include <iostream>
#include "serializer/stl_types.hpp"
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
    tmanager.Start();

    // Test the robustness of the client by spamming it
    {
      Client client(argv[1], argv[2], &tmanager);
      while(!client.is_alive()) {}
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      std::string test("Testing rapid string pushing ");
      for (std::size_t i = 0; i < 1000000; ++i)
      {
        test += "more size ";
        test += std::to_string(i);
      }

      for (std::size_t i = 0; i < 100; ++i)
      {
        fetch::byte_array::ByteArray msg0(test);
        client.Send(msg0.Copy());
      }

      while(client.HasOutgoingMessages()) { std::this_thread::sleep_for(std::chrono::milliseconds(1000));}
    }

    Client client(argv[1], argv[2], &tmanager);
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

