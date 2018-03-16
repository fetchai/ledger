#include"oef/fetch_node_service.hpp"
#include"commandline/parameter_parser.hpp"

using namespace fetch::fetch_node_service;
using namespace fetch::commandline;

int main(int argc, char const** argv) {

  ParamsParser params;
  params.Parse(argc, argv);

  if(params.IsParam("h") || params.IsParam("help")) {
    std::cout << "usage: ./" << argv[0] << " [params ...]" << std::endl;
    std::cout << "Params are:" << std::endl;
    std::cout << " -http_port=[8080]" << std::endl;
    std::cout << " -tcp_port=[9080]" << std::endl;
    std::cout << " -config_file=[./default_oef_config.txt]" << std::endl;
    std::cout << std::endl;

  }

  const uint16_t httpPort      = params.GetParam<uint16_t>("http_port", 8080);
  const uint16_t tcpPort       = params.GetParam<uint16_t>("tcp_port", 9080);
  const std::string configFile = params.GetParam<std::string>("config_file", "./default_oef_config.txt");

  fetch::network::ThreadManager tm(8);
  FetchNodeService serv(&tm, tcpPort, httpPort, configFile);

  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
