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

std::vector<std::string> getLocation(int location) {

  if(location-- == 0) { return {"Milngavie" , std::to_string(55.9425559), std::to_string(-4.3617068 )}; }
  if(location-- == 0) { return {"Edinburgh" , std::to_string(55.9411884), std::to_string(-3.2755497 )}; }
  if(location-- == 0) { return {"Cambridge" , std::to_string(52.1988369), std::to_string(0.084882   )}; }
  if(location-- == 0) { return {"Hull"      , std::to_string(53.7663502), std::to_string(-0.4021968 )}; }
  if(location-- == 0) { return {"Bath"      , std::to_string(51.3801212), std::to_string(-2.3996352 )}; }
  if(location-- == 0) { return {"Penzance"  , std::to_string(50.1195696), std::to_string(-5.5606844 )}; }
  if(location-- == 0) { return {"Skye"      , std::to_string(57.3617192), std::to_string(-6.7797837 )}; }
  if(0          == 0) { return {"Norwich"   , std::to_string(52.6401222), std::to_string(1.2166384  )}; }
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
    schema::Instance instance{node, {{"name", getLocation(seed)[0]}, {"latitude", getLocation(seed)[1]}, {"longitude", getLocation(seed)[2]}}};

    // this node's endpoint
    schema::Endpoint nodeEndpoint{"localhost", 9080+seed};

    // bootstrap endpoints
    std::set<schema::Endpoint> endp;

    // All endpoints get default (9080) plus one other (9081 -> current)
    endp.insert({"localhost", 9080});
    if(seed != 0) {
      endp.insert({"localhost", 9080 + (uint16_t(lfg()) % seed)});
    }

    //schema::Endpoint bootstrap;
    schema::Endpoints endpoints{endp};

    FetchNodeService serv(tm, 9080+seed, 8080+seed, instance, nodeEndpoint, endpoints);

    tm->Start();
    serv.Start();

    while(1) {};
}

int main(int argc, char const** argv) {

  fetch::network::ThreadManager tm(5);

  if(argc > 1) {
    int seed;
    std::stringstream s(argv[1]);
    s >> seed;

    std::thread thread{&runNode, seed, &tm};

    while(1) {};
  }

  /*
  std::vector<std::thread> testNodes;
  for (int i = 0; i < 10; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    testNodes.push_back(std::thread{&runNode, i, &tm});
  }*/

  while(1) {}
}
