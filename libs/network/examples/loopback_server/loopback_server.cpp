#include "network/tcp/loopback_server.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
  {
    std::cerr << "Starting loopback server" << std::endl;
    fetch::network::LoopbackServer echo(8080);

    char dummy;
    std::cout << "press any key to quit" << std::endl;
    std::cin >> dummy;
  }

  std::cout << "Finished" << std::endl;
  return 0;
}
