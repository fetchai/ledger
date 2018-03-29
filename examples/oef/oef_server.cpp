#include"oef/fetch_node_service.hpp"
#include"commandline/parameter_parser.hpp"
#include"oef/schema.hpp"

using namespace fetch::schema;
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
    std::cout << std::endl;
  }

  const uint16_t httpPort      = params.GetParam<uint16_t>("http_port", 8080);
  const uint16_t tcpPort       = params.GetParam<uint16_t>("tcp_port", 9080);

  fetch::network::ThreadManager tm(8);

  // Need an Instance for the Node too
  Attribute name        { "name",             Type::String, true}; // guarantee all DMs have this

  // We then create a DataModel->Instance for this
  std::string dmName = "nodeDM";
  DataModel generatedDM{dmName, {name}};

  Instance instance{generatedDM, {{"name", "node"}}};

  // no need for endpoins for non-distributed example
  const Endpoint dummy1{};
  const Endpoints dummy2{};

  FetchNodeService serv(&tm, tcpPort, httpPort, instance, dummy1, dummy2);

  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;
}
