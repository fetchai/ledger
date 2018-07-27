#include <functional>
#include <iostream>

#include "core/commandline/parameter_parser.hpp"

#include "shard_service.hpp"

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

using namespace fetch::protocols;
using namespace fetch::commandline;
using namespace fetch::byte_array;

FetchShardService *service;
void               StartService(int port)
{
  service = new FetchShardService(port);
  service->Start();
}

int main(int argc, char const **argv)
{
  ParamsParser params;
  params.Parse(argc, argv);

  if (params.arg_size() < 2)
  {
    std::cout << "usage: " << argv[0] << " [port]" << std::endl;
    exit(-1);
  }

  uint16_t my_port = params.GetArg<uint16_t>(1);
  StartService(my_port);

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  std::cout << "Press Ctrl+C to stop" << std::endl;
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  service->Stop();
  delete service;

  return 0;
}
