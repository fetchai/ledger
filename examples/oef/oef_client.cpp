#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"logger.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"protocols/fetch_protocols.hpp"
#include"protocols/aea_to_node/commands.hpp"
#include"protocols/node_to_aea.hpp"

using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;
using namespace fetch::protocols;

// Example of OEF code performing basic register-query functionality

int main() {

  // Client setup
  fetch::network::ThreadManager tm;
  ServiceClient< fetch::network::TCPClient > client("localhost", 8090, &tm);
  tm.Start();

  std::this_thread::sleep_for( std::chrono::milliseconds(100) );

  // Define attributes that can exist
  schema::Attribute wind        { "has_wind_speed",   schema::Type::Bool, false};
  schema::Attribute temperature { "has_temperature",  schema::Type::Bool, true};
  schema::Attribute latitude    { "latitude",         schema::Type::Bool, true};
  schema::Attribute longitude   { "longitude",        schema::Type::Bool, true};

  // We then create a DataModel for this, personalise it by creating an Instance,
  // and register it with the Node (connected during Client construction)
  std::vector<schema::Attribute> attributes{wind, temperature, latitude, longitude};

  // Create a DataModel
  schema::DataModel weather{"weather_data", attributes};

  // Create an Instance of this DataModel
  schema::Instance instance{weather, {{"has_wind_speed", "false"}, {"has_temperature", "true"}, {"latitude", "true"}, {"longitude", "true"}}};

  // Register our datamodel
  std::cout << client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::REGISTER_INSTANCE, "listening_agent", instance ).As<std::string>( ) << std::endl;

  // Register ourself for callbacks
  NodeToAEAProtocol protocol;
  protocol.onPing() = [&](std::string message){ std::cerr << "We received a callback ping: " << message << std::endl;};

  // Sell bananas callback
  int bananas = 4;
  fetch::mutex::Mutex banana_mutex;
  protocol.onBuy() = [&](std::string fromPerson){

    std::lock_guard< fetch::mutex::Mutex > lock( banana_mutex );

    if(bananas == 0) {
      return std::string{"we have no bananas"};
    }

    bananas--;

    return std::string{"we have bananas"};
  };

  client.Add(FetchProtocols::NODE_TO_AEA, &protocol);

  auto p =  client.Call(FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::REGISTER_FOR_CALLBACKS, "listening_agent");

  if(p.Wait() ) {
    std::cout << "Successfully registered for callbacks" << std::endl;
  }

  // Now we can wait for people to poke us
  while(bananas > 0) {std::chrono::milliseconds(100);}

  std::cout << "Sold all our bananas, exit" << std::endl;

  auto p2 =  client.Call(FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::DEREGISTER_FOR_CALLBACKS, "listening_agent");

  p2.Wait();

  tm.Stop();
  return 0;
}
