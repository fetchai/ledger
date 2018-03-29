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
  schema::Attribute latitude{ "latitude",        schema::Type::Float, true}; // guarantee all DMs have this

  // Create constraints against our Attribute(s) (latitude less than 50.70)
  schema::ConstraintType customConstraint{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Lt, float(50.70)}}};
  schema::Constraint latitude_c{ latitude,    customConstraint};

  // Query is build up from constraints
  schema::QueryModel query1{{latitude_c}};

  // query the oef for a list of agents
  auto agents = client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY, "seb", query1 ).As<std::vector<std::string>>( );

  std::cout << "query result: " << std::endl;
  for(auto i : agents){
    std::cout << i << std::endl;
  }

  // Try to buy from those agents, quite a lot of times (we are expecting them to eventually run out of whatever they're selling)
  for (int i = 0; i < 100; ++i) {
    for(auto &agent : agents){
      std::cout << "Attempting to buy from: " << agent << std::endl;
      std::cout << "result is " << client.Call(FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::BUY, "seb", agent ).As<std::string>() << std::endl;

      std::this_thread::sleep_for( std::chrono::milliseconds(1000));
    }
  }

  tm.Stop();
  return 0;
}
