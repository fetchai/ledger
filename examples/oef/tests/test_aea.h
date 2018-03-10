#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"logger.hpp"
#include"oef/service_consts.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"oef/node_to_aea_protocol.hpp"
#include"random/lfg.hpp"

using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;
using namespace fetch::random;
using namespace fetch::node_to_aea_protocol;

class TestAEA {

public:
  TestAEA(int randomSeed) {

    AEA_name_ = std::string("aea_") + std::to_string(randomSeed);
    thread_   = std::make_unique<std::thread>([this]() { run(); });
  }

private:
  int                                  randomSeed_;
  std::unique_ptr<std::thread>         thread_;
  std::string                          AEA_name_;

  void run(){
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
    std::cout << client.Call( AEAToNodeProtocolID::DEFAULT, AEAToNodeProtocolFn::REGISTER_INSTANCE, AEA_name_, instance ).As<std::string>( ) << std::endl;

    // Register ourself for callbacks
    NodeToAEAProtocol protocol;
    protocol.onPing() = [&](std::string message){ std::cerr << "We received a callback ping: " << message << std::endl;};

    // Sell bananas callback
    int bananas = (randomSeed_ % 20) + 1;
    fetch::mutex::Mutex banana_mutex;
    protocol.onBuy() = [&](std::string fromPerson){

      std::lock_guard< fetch::mutex::Mutex > lock( banana_mutex );

      if(bananas == 0) {
        return std::string{"we have no bananas"};
      }

      bananas--;

      return std::string{"we have bananas"};
    };

    client.Add(NodeToAEAProtocolID::DEFAULT_ID, &protocol);

    auto p =  client.Call(AEAToNodeProtocolID::DEFAULT, AEAToNodeProtocolFn::REGISTER_FOR_CALLBACKS, AEA_name_);

    if(p.Wait() ) {
      std::cout << "Successfully registered for callbacks" << std::endl;
    }

    // Now we can wait for people to poke us
    while(bananas > 0) {std::chrono::milliseconds(100);}

    std::cout << "Sold all our bananas, exit" << std::endl;

    auto p2 =  client.Call(AEAToNodeProtocolID::DEFAULT, AEAToNodeProtocolFn::DEREGISTER_FOR_CALLBACKS, AEA_name_);

    p2.Wait();

    tm.Stop();

  }
};
