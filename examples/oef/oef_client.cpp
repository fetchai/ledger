#include"oef_service_consts.hpp"
#include<iostream>
#include"serializer/referenced_byte_array.hpp"
#include"service/client.hpp"
#include"logger.hpp"
#include"oef/schema.h"
#include"oef/schemaSerializers.h"
using namespace fetch::service; // TODO: (`HUT`) : remove namespaces
using namespace fetch::byte_array;

// Example of OEF code performing basic register-query functionality

int main() {

  // Client setup
  fetch::network::ThreadManager tm;
  ServiceClient< fetch::network::TCPClient > client("localhost", 8080, &tm);
  tm.Start();

  std::this_thread::sleep_for( std::chrono::milliseconds(100) );

  // Define attributes that can exist
  Attribute wind        { "has_wind_speed",   Type::Bool, false};
  Attribute temperature { "has_temperature",  Type::Bool, true};
  Attribute air         { "has_air_pressure", Type::Bool, true};
  Attribute humidity    { "has_humidity",     Type::Bool, true};
  Attribute latitude    { "latitude",         Type::Bool, true};
  Attribute longitude   { "longitude",        Type::Bool, true};

  // We then create a DataModel for this, personalise it by creating an Instance,
  // and register it with the Node (connected during Client construction)
  std::vector<Attribute> attributes{wind, temperature, air, humidity, latitude, longitude};

  // Create a DataModel
  DataModel weather{"weather_data", attributes};

  // Create an Instance of this DataModel
  Instance instance{weather, {{"wind_speed", "false"}, {"has_temperature", "true"}, {"has_air_pressure", "true"}, {"has_humidity", "true"}, {"latitude", "true"}, {"longitude", "true"}}};

  // Register our datamodel
  std::cout << client.Call( MYPROTO, REGISTERDATAMODEL, "test_agent", instance ).As<std::string>( ) << std::endl;

  // two queries, first one should succeed, one should fail since we are searching for wind and temperature with our agent has/does not have

  // Create constraints against our Attributes (whether the station CAN provide them)
  ConstraintType eqTrue{ConstraintType::ValueType{Relation{Relation::Op::Eq, true}}};
  Constraint temperature_c { temperature, eqTrue};
  Constraint wind_c        { wind    ,    eqTrue};

  // Query is build up from constraints
  QueryModel query1{{temperature_c}};
  QueryModel query2{{wind_c}};

  // first query, should succeed (searching for has_temperature)
  auto agents = client.Call( MYPROTO, QUERY, "querying_agent", query1 ).As<std::vector<std::string>>( );

  std::cout << "first query result: " << std::endl;
  for(auto i : agents){
    std::cout << i << std::endl;
  }

  // second query, should fail (searching for wind_speed)
  agents = client.Call( MYPROTO, QUERY, "querying_agent", query2 ).As<std::vector<std::string>>( );

  std::cout << "second query result: " << std::endl;
  for(auto i : agents){
    std::cout << i << std::endl;
  }

  tm.Stop();
  return 0;
}
