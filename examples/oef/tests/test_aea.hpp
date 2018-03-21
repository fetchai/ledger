#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"logger.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"random/lfg.hpp"
#include"protocols/fetch_protocols.hpp"
#include"protocols/aea_to_node/commands.hpp"
#include"protocols/node_to_aea/protocol.hpp"

using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;
using namespace fetch::random;
using namespace fetch::protocols;

class TestAEA {

public:
  TestAEA()  = delete;
  TestAEA(TestAEA const &rhs)  = delete;
  TestAEA(TestAEA const &&rhs)  = delete;

  TestAEA(uint32_t randomSeed, uint16_t portNumber=9080) : randomSeed_{randomSeed}, portNumber_{portNumber} {

    std::ostringstream name;
    name << std::string("aea_") << std::to_string(portNumber_) << std::string("_") << std::to_string(randomSeed & 0xFFFF);
    AEA_name_ = name.str();

    std::cout << "Connecting AEA: " << AEA_name_ << std::endl;

    thread_   = std::make_unique<std::thread>([&]() { run(); });
  }

  bool isSetup() { return isSetup_; }

private:

  uint32_t                             randomSeed_;
  uint16_t                             portNumber_;
  std::unique_ptr<std::thread>         thread_;
  std::string                          AEA_name_;
  bool                                 isSetup_ = false;

  void run(){

    std::cout << "AEA name is " << AEA_name_ << std::endl;

    // Client setup
    fetch::network::ThreadManager tm;
    ServiceClient< fetch::network::TCPClient > client("localhost", portNumber_, &tm);
    tm.Start();

    std::this_thread::sleep_for( std::chrono::milliseconds(100) );

    // Define attributes that can exist
    schema::Attribute name        { "name",             schema::Type::String, true}; // guarantee all DMs have this
    schema::Attribute latitude    { "latitude",         schema::Type::Float, true}; // guarantee all DMs have this
    schema::Attribute longitude   { "longitude",        schema::Type::Float, true}; // guarantee all DMs have this
    schema::Attribute wind        { "has_wind_speed",   schema::Type::Bool, false};
    schema::Attribute temperature { "has_temperature",  schema::Type::Bool, false};
    schema::Attribute humidity    { "has_humidity",     schema::Type::Bool, false};
    schema::Attribute pressure    { "has_pressure",     schema::Type::Bool, false};

    // Use our random number to create a random DataModel using these attributes
    std::vector<schema::Attribute> possibleAttributes{wind, temperature, humidity, pressure};
    std::vector<schema::Attribute> usedAttributes{name, latitude, longitude};
    int random = randomSeed_;

    for (int i = 0; i < possibleAttributes.size(); ++i) {
      if(random & 0x1) {
        usedAttributes.push_back(possibleAttributes[i]);
      }
      random >>= 1;
    }

    // We then create a DataModel for this, use seed for name, note there should be no clashing DMs
    std::string dmName = std::string("gen_dm_") + std::to_string(randomSeed_ & ((2^possibleAttributes.size()) - 1));
    // Create a DataModel
    schema::DataModel generatedDM{dmName, usedAttributes};

    // Create an Instance of this DataModel
    std::unordered_map<std::string,std::string> attributeValues;

    // Set the hard-coded values
    attributeValues[usedAttributes[0].name()] = AEA_name_;
    attributeValues[usedAttributes[1].name()] = std::to_string(((randomSeed_) % 10000)/5000.2 + 50); // TODO: (`HUT`) : this could throw float exec
    attributeValues[usedAttributes[2].name()] = std::to_string(((randomSeed_) % 10000)/5000.1 + 1);

    for (int i = 3; i < usedAttributes.size(); ++i) {
      attributeValues[usedAttributes[i].name()] =  (randomSeed_ & (0x80 >> i)) ? "true" : "false";
    }

    schema::Instance instance{generatedDM, attributeValues};

    // Register our datamodel
    std::cout << client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::REGISTER_INSTANCE, AEA_name_, instance ).As<std::string>( ) << std::endl;

    // Register ourself for callbacks
    NodeToAEAProtocol protocol;
    protocol.onPing() = [&](std::string message){ std::cerr << "We received a callback ping: " << message << std::endl;};

    // Sell bananas callback
    int bananas = (randomSeed_ % 20) + 1;
    std::cout << "AEA " << AEA_name_ << " starting with " << bananas << " bananas!" << std::endl;

    fetch::mutex::Mutex banana_mutex;
    protocol.onBuy() = [&](std::string fromPerson){

      std::cout << "AEA " << AEA_name_ << "has been called back by " << fromPerson << std::endl;
      std::lock_guard< fetch::mutex::Mutex > lock( banana_mutex );

      if(bananas == 0) {
        return std::string{"we have no bananas"};
      }

      bananas--;

      return std::string{"we have bananas"};
    };

    client.Add(FetchProtocols::NODE_TO_AEA, &protocol);

    auto p =  client.Call(FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::REGISTER_FOR_CALLBACKS, AEA_name_, instance);

    if(p.Wait() ) {
      std::cout << "Successfully registered for callbacks" << std::endl;
    }

    isSetup_ = true;

    // Now we can wait for people to poke us
    while(bananas > 0) {std::chrono::milliseconds(100);}

    std::cout << "Sold all our bananas, exit" << std::endl;

    auto p2 =  client.Call(FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::DEREGISTER_FOR_CALLBACKS, AEA_name_);

    p2.Wait();

    tm.Stop();

  }
};
