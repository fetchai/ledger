#include"commandline/parameter_parser.hpp"
#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"random/lfg.hpp"
#include"./test_aea.hpp" // TODO: (`HUT`) : delete unnecc.

using namespace fetch;
using namespace fetch::service;
using namespace fetch::byte_array;
using namespace fetch::random;

// Example of OEF code performing basic register-query functionality

int main() {

  LaggedFibonacciGenerator<> lfg;
  std::vector<TestAEA *> testAEAs;

  // Register nine agents on three nodes (randomly chosen attributes)
  testAEAs.push_back(new TestAEA{uint32_t(lfg()), uint16_t(9080)});
  testAEAs.push_back(new TestAEA{uint32_t(lfg()), uint16_t(9081)});
  testAEAs.push_back(new TestAEA{uint32_t(lfg()), uint16_t(9082)});

  testAEAs.push_back(new TestAEA{uint32_t(lfg()), uint16_t(9080)});
  testAEAs.push_back(new TestAEA{uint32_t(lfg()), uint16_t(9081)});
  testAEAs.push_back(new TestAEA{uint32_t(lfg()), uint16_t(9082)});

  testAEAs.push_back(new TestAEA{uint32_t(lfg()), uint16_t(9080)});
  testAEAs.push_back(new TestAEA{uint32_t(lfg()), uint16_t(9081)});
  testAEAs.push_back(new TestAEA{uint32_t(lfg()), uint16_t(9082)});

  // Make sure they all connected properly
  for(auto &i : testAEAs) {
    while(!i->isSetup()) {}
  }

  // Create a query that should hit the first one only (forwarding query too difficult)
  std::string dummy;

  std::cout << "Press any key to query locally" << std::endl;
  std::cin >> dummy;
  std::cout << "Querying locally!" << std::endl;

  fetch::network::ThreadManager tm;
  ServiceClient< fetch::network::TCPClient > client("localhost", 9080, &tm);
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
  auto agents = client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY, "querying_agent", query1 ).As<std::vector<std::string>>( );

  for (int i = 0; i < 20; ++i) {
    client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY, "querying_agent", query1 ).As<std::vector<std::string>>( );
  }

  std::cout << "query result: " << std::endl;
  for(auto i : agents){
    std::cout << i << std::endl;
  }

  std::cout << "Press ENTER to query distributed " << std::endl;
  //{"Milngavie" , lat(55.9425559), long(-4.3617068 )} // 8080
  //{"Edinburgh" , lat(55.9411884), long(-3.2755497 )} // 8081
  //{"Cambridge" , lat(52.1988369), long(0.084882   )} // 8082

  while(1) {}
}
