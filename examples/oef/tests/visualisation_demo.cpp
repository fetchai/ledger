#include"commandline/parameter_parser.hpp"
#include"logger.hpp"
#include"oef/fetch_node_service.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"protocols/aea_to_node/commands.hpp"
#include"protocols/fetch_protocols.hpp"
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"random/lfg.hpp"
#include<iostream>
#include<tuple>

#include"protocols/node_to_node/commands.hpp" // TODO: (`HUT`) : remove this

using namespace fetch::byte_array;
using namespace fetch::commandline;
using namespace fetch::fetch_node_service;
using namespace fetch::protocols;
using namespace fetch::service;
using namespace fetch::random;
using namespace fetch;

std::vector<std::string> getLocation(int select, uint32_t random) {

  // Possible location names
  std::vector<std::string> list = {"destination_A", "destination_B", "destination_C", "destination_D", "destination_E", "destination_F", "destination_G"};
  std::cout << "Random is " << random << "for select" << select << std::endl;

  // Center
  float base_lat = 51.5090446;
  float base_lng = -0.0993713;

  float lat = base_lat + ((float(random % 100) - 50)*0.001) * 0.001;
  float lng = base_lng + ((float(random % 111) - 50)*0.001) * 0.001;;

  return {list[select % list.size()], std::to_string(lat), std::to_string(lng)};
}

void runNode(int seed, network::ThreadManager *tm) {

    fetch::logger.Debug("Constructing node: ", seed);
    LaggedFibonacciGenerator<> lfg{seed};

    schema::Attribute id          { "name",   schema::Type::String, true};
    schema::Attribute latitude    { "latitude",         schema::Type::Float, true};
    schema::Attribute longitude   { "longitude",        schema::Type::Float, true};

    std::vector<schema::Attribute> attributes{id, latitude, longitude};

    // Create a DataModel
    schema::DataModel node{"node", attributes};

    // Create an Instance of this DataModel
    uint32_t random = uint32_t(rand() * (seed+1));
    schema::Instance instance{node, {{"name", getLocation(seed, random)[0]}, {"latitude", getLocation(seed, random)[1]}, {"longitude", getLocation(seed, random)[2]}}};

    // this node's endpoint
    schema::Endpoint nodeEndpoint("localhost", 9080+seed);

    // bootstrap endpoints
    std::set<schema::Endpoint> endp;

    // All endpoints get default (9080) plus one other (9081 -> current)
    endp.insert({"localhost", uint16_t(9080)});
    if(seed != 0) {
      for (int i = 0; i < seed; ++i) { rand(); } // weakly ensure rand is different for each seed
      uint16_t port = 9080 + (uint16_t(rand()) % seed);
      endp.insert({"localhost", port});
    }

    //schema::Endpoint bootstrap;
    schema::Endpoints endpoints{endp};

    FetchNodeService serv(tm, 9080+seed, 8080+seed, instance, nodeEndpoint, endpoints);

    tm->Start();
    serv.Start();

    while(1) {};
}

int main(int argc, char const** argv) {

  fetch::network::ThreadManager tm(10);

  if(argc > 1) {
    int seed;
    std::stringstream s(argv[1]);
    s >> seed;

    std::thread thread{&runNode, seed, &tm};

    while(1) {};
  }

  while(1) {}
}
