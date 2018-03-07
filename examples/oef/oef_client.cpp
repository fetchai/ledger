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
  std::cout << client.Call( AEAProtocolEnum::DEFAULT, AEAProtocol::REGISTER_INSTANCE, "test_agent", instance ).As<std::string>( ) << std::endl;

  // two queries, first one should succeed, one should fail since we are searching for wind and temperature with our agent has/does not have

  // Create constraints against our Attributes (whether the station CAN provide them)
  schema::ConstraintType eqTrue{schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Eq, true}}};
  schema::Constraint temperature_c { temperature, eqTrue};
  schema::Constraint wind_c        { wind    ,    eqTrue};

  // Query is build up from constraints
  schema::QueryModel query1{{temperature_c}};
  schema::QueryModel query2{{wind_c}};

  // first query, should succeed (searching for has_temperature)
  auto agents = client.Call( AEAProtocolEnum::DEFAULT, AEAProtocol::QUERY, query1 ).As<std::vector<std::string>>( );

  std::cout << "first query result: " << std::endl;
  for(auto i : agents){
    std::cout << i << std::endl;
  }

  // second query, should fail (searching for wind_speed)
  agents = client.Call( AEAProtocolEnum::DEFAULT, AEAProtocol::QUERY, query2 ).As<std::vector<std::string>>( );

  std::cout << "second query result: " << std::endl;
  for(auto i : agents){
    std::cout << i << std::endl;
  }

  tm.Stop();
  return 0;
}
