#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"logger.hpp"
#include"oef/service_consts.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"

using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;

// Example of OEF code performing basic register-query functionality

int main() {

  // Client setup
  fetch::network::ThreadManager tm;
  ServiceClient< fetch::network::TCPClient > client("localhost", 8090, &tm);
  tm.Start();

  std::this_thread::sleep_for( std::chrono::milliseconds(100) );

  // The attribute we want to search for
  schema::Attribute longitude   { "longitude",        schema::Type::Bool, true};

  // Create constraints against our Attribute(s) (whether the AEA CAN provide them in this case)
  schema::ConstraintType eqTrue{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Eq, true}}};
  schema::Constraint longitude_c   { longitude    ,    eqTrue};

  // Query is build up from constraints
  schema::QueryModel query1{{longitude_c}};

  // query the oef for a list of agents
  auto agents = client.Call( AEAToNodeProtocolID::DEFAULT, AEAToNodeProtocolFn::QUERY, query1 ).As<std::vector<std::string>>( );

  std::cout << "query result: " << std::endl;
  for(auto i : agents){
    std::cout << i << std::endl;
  }

  // Try to buy from those agents
  for (int i = 0; i < 100; ++i) {
    for(auto &agent : agents){
      std::cout << "Attempting to buy from: " << agent << std::endl;
      std::cout << "result is " << client.Call(AEAToNodeProtocolID::DEFAULT, AEAToNodeProtocolFn::BUY_AEA_TO_NODE, agent ).As<std::string>() << std::endl;

      std::this_thread::sleep_for( std::chrono::milliseconds(1000));
    }
  }

  tm.Stop();
  return 0;
}
