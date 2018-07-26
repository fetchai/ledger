#include "network/tcp/tcp_server.hpp"
#include <cstdlib>
#include <iostream>
using namespace fetch::network;

class Server : public TCPServer
{
public:
  Server(uint16_t p, NetworkManager tmanager) : TCPServer(p, tmanager) {}

  void PushRequest(connection_handle_type client,
                   message_type const &   msg) override
  {
    std::cout << "Message: " << msg << std::endl;
  }

private:
};

int main(int argc, char *argv[])
{
  try
  {

    if (argc != 2)
    {
      std::cerr << "Usage: rpc_server <port>\n";
      return 1;
    }

    NetworkManager tmanager;

    Server s(uint16_t(std::atoi(argv[1])), tmanager);
    tmanager.Start();

    std::cout << "Press Ctrl+C to quit" << std::endl;

    while (true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    tmanager.Stop();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
