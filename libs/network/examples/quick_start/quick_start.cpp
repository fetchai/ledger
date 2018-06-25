#include<iostream>

#include"core/commandline/parameter_parser.hpp"
#include"core/commandline/vt100.hpp"

#include"network/details/thread_manager.hpp"
#include"./quick_start_service.hpp"

using namespace fetch;
using namespace fetch::commandline;
using namespace fetch::quick_start;

int main(int argc, char const **argv)
{
  // Networking needs this
  fetch::network::ThreadManager tm(5);

  ParamsParser params;
  params.Parse(argc, argv);

  // get our port and the port of the remote we want to connect to
  if(params.arg_size() <= 1) {
    std::cout << "usage: ./" << argv[0] << " [params ...]" << std::endl;
    std::cout << std::endl << "Params are" << std::endl;
    std::cout << "  --port=[8000]" << std::endl;
    std::cout << "  --remotePort=[8001]" << std::endl;
    std::cout << std::endl;

    return -1;
  }

  uint16_t tcpPort  = uint16_t(std::stoi(params.GetArg(1)));
  uint16_t remotePort  = uint16_t(std::stoi(params.GetArg(2)));
  std::cout << "Starting server on tcp: " << tcpPort 
    << " connecting to: " << remotePort << std::endl;

  // Start our service
  QuickStartService serv(tm, tcpPort);
  tm.Start();

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::cout << "Enter message to send to connected node(s)" << std::endl;
    std::string message;
    std::getline(std::cin, message);
    serv.sendMessage(message, remotePort);
  }

  tm.Stop();
}
