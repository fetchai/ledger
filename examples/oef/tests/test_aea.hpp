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
  TestAEA(int randomSeed) : randomSeed_{randomSeed} {

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
    schema::Attribute name        { "has_name",         schema::Type::Bool, true}; // guarantee all DMs have this
    schema::Attribute wind        { "has_wind_speed",   schema::Type::Bool, false};
    schema::Attribute temperature { "has_temperature",  schema::Type::Bool, false};
    schema::Attribute latitude    { "latitude",         schema::Type::Bool, false};
    schema::Attribute longitude   { "longitude",        schema::Type::Bool, false};

    // Use our random number to create a random DataModel using these attributes
    std::vector<schema::Attribute> possibleAttributes{wind, temperature, latitude, longitude};
    std::vector<schema::Attribute> usedAttributes{name};
    int random = randomSeed_;

    for (int i = 0; i < possibleAttributes.size(); ++i) {
      if(random & 0x1) {
        usedAttributes.push_back(possibleAttributes[i]);
      }
      random >>= 1;
    printf("rrr %x\n\n\n", random);
    }

    printf("test %x\n\n\n", ((2^possibleAttributes.size()) - 1));

    // We then create a DataModel for this, use seed for name, note there should be no clashing DMs
    std::string dmName = std::string("gen_dm_") + std::to_string(randomSeed_ & ((2^possibleAttributes.size()) - 1));
    // Create a DataModel
    schema::DataModel generatedDM{dmName, usedAttributes};

    // Create an Instance of this DataModel
    std::unordered_map<std::string,std::string> attributeValues;
    for (int i = 0; i < usedAttributes.size(); ++i) {
      attributeValues[usedAttributes[i].name()] =  (randomSeed_ & (0x80 >> i)) ? "true" : "false";
    }

    schema::Instance instance{generatedDM, attributeValues};

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
