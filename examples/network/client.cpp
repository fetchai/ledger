#include <cstdlib>
#include <iostream>
#include"network/network_client.hpp"
using namespace fetch::network;

class Client : public NetworkClient {
public:
  Client(std::string const &host,
         std::string const &port)
    : NetworkClient(host, port ) { }
  
  void PushMessage(message_type const &value) override {
    std::cout << value << std::endl;
  }
};

int main(int argc, char* argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }

    Client client(argv[1], argv[2]);
    client.Start();
    
    fetch::byte_array::ByteArray msg;
    msg.Resize(512);

    while (std::cin.getline(msg.char_pointer(), 512)) {
      msg.Resize( std::strlen(msg.char_pointer()) );
      client.Send(msg);
      msg.Resize(512);
    }

    client.Stop();

  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;  
}

