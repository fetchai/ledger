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

// Example of OEF code performing basic register-query functionality (needs updating - lat longs are out of date)
// Note: Three OEF nodes must be running on localhost, ports 9080, 9081, 9082

int main() {

  LaggedFibonacciGenerator<> lfg(1);
  std::vector<TestAEA *> testAEAs;

  // Register agents on X nodes (randomly chosen attributes)
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9080)});
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9081)});
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9082)});

  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9080)});
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9081)});
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9082)});

  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9080)});
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9081)});
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9082)});

  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9081)});
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9081)});
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9081)});
  testAEAs.push_back(new TestAEA{uint32_t(rand()), uint16_t(9081)});

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
  ServiceClient< fetch::network::TCPClient > client1("localhost", 9082, &tm);
  tm.Start();

  std::this_thread::sleep_for( std::chrono::milliseconds(100) );

  // The attribute we want to search for
  schema::Attribute longitude { "longitude", schema::Type::Float, true};
  schema::Attribute latitude  { "latitude",  schema::Type::Float, true};

  {
    // Create constraints against our Attribute(s) (whether the AEA CAN provide them in this case)
    schema::ConstraintType constraint{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Lt, float(2.1)}}};
    schema::Constraint longitude_c   { longitude    ,    constraint};

    // Query is build up from constraints
    schema::QueryModel query{{longitude_c}};

    // query the oef for a list of agents
    auto agents = client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY, "querying_agent", query ).As<std::vector<std::string>>( );

    // TODO: (`HUT`) : remove this, it's only for fillng up the event log
    for (int i = 0; i < 5; ++i) {
      client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY, "querying_agent", query ).As<std::vector<std::string>>( );
    }

    // this should only return aea_9080_37962
    std::cout << "query result: " << std::endl;
    for(auto i : agents){
      std::cout << i << std::endl;
    }
  }

  // reminder
  //{"Milngavie" , lat(55.9425559), long(-4.3617068 )} // 8080
  //{"Edinburgh" , lat(55.9411884), long(-3.2755497 )} // 8081
  //{"Cambridge" , lat(52.1988369), long(0.084882   )} // 8082
  // Now we make a MULTI-NODE QUERY that we expect to fail for the node 8080
  {
    // Same constraint as before
    schema::ConstraintType constraint{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Lt, float(2.1)}}};
    schema::ConstraintType forwardingConstraint{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Lt, float(-5.5)}}};
    schema::Constraint     longitude_c      { longitude, constraint};
    schema::Constraint     forw_longitude_c { longitude, forwardingConstraint};

    schema::QueryModel      query{{longitude_c}};
    schema::QueryModel      forwardingQuery{{forw_longitude_c}};
    schema::QueryModelMulti queryMulti{query, forwardingQuery, 1}; // note only ONE jump allowed

    // Create a forwarding query (want this to fail)
    auto agents = client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY_MULTI, "querying_agent", queryMulti ).As<std::vector<std::string>>( );

    std::cout << "second query result (expect fail): " << std::endl << std::endl;
    for(auto i : agents){
      std::cout << i << std::endl;
    }

    // Now MATCH our first node's parameters
    schema::ConstraintType  pass{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Gt, float(-5.5)}}};
    schema::Constraint      pass_forw { longitude, pass};
    schema::QueryModel      passQuery{{pass_forw}};
    schema::QueryModelMulti passQueryMulti{query, passQuery};

    // Create a forwarding query (want this to fail)
    agents = client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY_MULTI, "querying_agent", passQueryMulti ).As<std::vector<std::string>>( );

    std::cout << "third query result (expect pass aea_9080_37962): " << std::endl;
    for(auto i : agents){
      std::cout << i << std::endl;
    }

    // try this again (should fail due to duplicated packet)
    agents = client.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY_MULTI, "querying_agent", passQueryMulti ).As<std::vector<std::string>>( );

    std::cout << "fourth query result (expect fail): " << std::endl;
    for(auto i : agents){
      std::cout << i << std::endl;
    }
  }

  std::cout << "Press ENTER to query distributed " << std::endl;
  std::cin >> dummy;

  // reminder
  //{"Milngavie" , lat(55.9425559), long(-4.3617068 )} // 8080
  //{"Edinburgh" , lat(55.9411884), long(-3.2755497 )} // 8081
  //{"Cambridge" , lat(52.1988369), long(0.084882   )} // 8082
  // Now for a truly distributed query: from node 9082 we want to hit node 9080 only
  {
    // Same constraint as before
    schema::ConstraintType longConst   { schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Gt, float(-4.0)}}};
    schema::ConstraintType latConst    { schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Lt, float(55.942)}}}; // should only hit Edinburgh and Cambridge

    // see if any of them have humidity
    schema::Attribute humidity         { "has_humidity",     schema::Type::Bool, false};
    schema::ConstraintType AEAHumConst { schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Eq, true}}};

    schema::Constraint longitude_c      { longitude, longConst};
    schema::Constraint latitude_c       { latitude, latConst};
    schema::Constraint ATALatConst_c    { humidity, AEAHumConst};

    schema::QueryModel      query{{ATALatConst_c}};
    schema::QueryModel      forwardingQuery{{longitude_c, latitude_c}};
    schema::QueryModelMulti queryMulti{query, forwardingQuery};

    // Note different client this point
    auto agents = client1.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY_MULTI, "querying_agent", queryMulti ).As<std::vector<std::string>>( );

    std::cout << "third query result (expect multi): " << std::endl;
    for(auto i : agents){
      std::cout << i << std::endl;
    }
  }

  // Query for all agents by not specifying constraints
  {
    schema::QueryModel      query{};
    schema::QueryModel      forwardingQuery{};
    schema::QueryModelMulti queryMulti{query, forwardingQuery};

    // Note different client this point
    auto agents = client1.Call( FetchProtocols::AEA_TO_NODE, AEAToNodeRPC::QUERY_MULTI, "querying_agent", queryMulti ).As<std::vector<std::string>>( );

    std::cout << "third query result (expect all): " << std::endl;
    for(auto i : agents){
      std::cout << i << std::endl;
    }
  }

  std::cout << "Finished, exit" << std::endl;

  for(auto i : testAEAs) {
    delete i;
  }
}
