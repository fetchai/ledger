#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"logger.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"protocols/fetch_protocols.hpp"
#include"protocols/aea_to_node/commands.hpp"

using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;
using namespace fetch::protocols;

// Example of OEF code performing basic register-query functionality

int main() {

  // Client setup
  fetch::network::ThreadManager tm;
  ServiceClient< fetch::network::TCPClient > client("localhost", 9080, &tm);
  tm.Start();

  std::this_thread::sleep_for( std::chrono::milliseconds(100) );

  // The attribute we want to search for
  schema::Attribute longitude   { "longitude",        schema::Type::Float, true}; // guarantee all DMs have this

  // Create constraints against our Attribute(s) (whether the AEA CAN provide them in this case)
  schema::ConstraintType customConstraint{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Lt, float(100.01)}}};
  schema::Constraint longitude_c   { longitude    ,    customConstraint};

  // Query is build up from constraints
  schema::QueryModel query1{{longitude_c}};

  // query the oef for a list of agents
  auto agents = client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY, std::string("seb"), query1 ).As<std::vector<std::string>>( );

  std::cout << "query result: " << std::endl;
  for(auto i : agents){
    std::cout << i << std::endl;
  }

  // Try to buy from those agents
  for (int i = 0; i < 100; ++i) {
    for(auto &agent : agents){
      std::cout << "Attempting to buy from: " << agent << std::endl;
      std::cout << "result is " << client.Call(FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::BUY, agent ).As<std::string>() << std::endl;

      std::this_thread::sleep_for( std::chrono::milliseconds(1000));
    }
  }

  tm.Stop();
  return 0;
}
