#include<iostream>

#include"network/management/network_manager.hpp"
#include"./subscribe_service.hpp"

using namespace fetch;
using namespace fetch::subscribe;

int main(int argc, char const **argv)
{
  // Networking needs this
  fetch::network::NetworkManager tm(5);

  uint16_t tcpPort  = 8080;
  std::cout << "Starting subscribe server on tcp: " << tcpPort << std::endl;

  // Start our service
  SubscribeService serv(tm, tcpPort);
  tm.Start();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cout << "Enter message to send to connected client(s)" << std::endl;
    std::string message;
    std::getline(std::cin, message);
    serv.SendMessage(message);
  }

  tm.Stop();
}
