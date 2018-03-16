#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"logger.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"protocols/fetch_protocols.hpp"
#include"protocols/aea_to_node/commands.hpp"

#include"protocols/node_to_node/commands.hpp" // TODO: (`HUT`) : remove this

using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;
using namespace fetch::protocols;

// Example of OEF code performing basic register-query functionality

int main() {

  // Client setup
  fetch::network::ThreadManager tm;
  ServiceClient< fetch::network::TCPClient > client("localhost", 9080, &tm);
  ServiceClient< fetch::network::TCPClient > client2("localhost", 9081, &tm);
  tm.Start();

  std::this_thread::sleep_for( std::chrono::milliseconds(100) );

  // The attribute we want to search for
  schema::Attribute hasCats   { "has_cats",        schema::Type::Bool, true};

  // Create constraints against our Attribute(s) (whether the AEA CAN provide them in this case)
  schema::ConstraintType eqTrue{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Eq, true}}};
  schema::Constraint hasCats_c   { hasCats    ,    eqTrue};

  // Query is build up from constraints
  schema::QueryModel query1{{hasCats_c}};

  // Need to specify network forwarding constraints
  schema::ConstraintType  lessThan300 {schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Gt, 2}}};
  schema::Attribute       longitude   { "longitude",        schema::Type::Int, true};
  schema::Constraint      longitude_c   { longitude    ,    lessThan300};
  schema::QueryModel      forwardingModel{{longitude_c}};

  // Test we are sending the right thing
  std::ostringstream message;
  message << query1.variant() << std::endl << std::endl;
  message << forwardingModel.variant();

  std::cout << message.str() << std::endl;

  schema::QueryModelMulti      multi{query1, forwardingModel};

  // query the oef for a list of agents
  auto agents = client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY_MULTI, multi).As<std::vector<std::string>>( );

  std::cout << "query result: " << std::endl;
  for(auto i : agents){
    std::cout << i << std::endl;
  }

  tm.Stop();
  return 0;
}
