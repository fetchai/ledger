#include <cstdlib>
#include <iostream>
#include "serializers/stl_types.hpp"
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

    for (std::size_t i = 0; i < 1000; ++i)
    {
      std::cerr << "Create tm" << std::endl;
      ThreadManager tmanager;
      tmanager.Start();

      for (std::size_t j = 0; j < 1; ++j)
      {
        std::cerr << "Create client" << std::endl;
        Client client(argv[1], argv[2], &tmanager);
        std::cerr << "Created client: " << i << ":" << j << std::endl;
      }

      tmanager.Stop();
    }

  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

