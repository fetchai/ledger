#include"oef_service_consts.hpp"
#include<iostream>
#include"service/server.hpp"

#include "old_oef_codebase/lib/include/serviceDirectory.h"
using namespace fetch::service;
using namespace fetch::byte_array;


// First we make a service implementation
class Implementation {

public:
  int SlowFunction(int a, int b) {
    std::this_thread::sleep_for( std::chrono::milliseconds(20) );
    return a + b;
  }

  int Add(int a, int b) {
    return a + b;
  }

  std::string Greet(std::string name) {
    return "Hello, " + name;
  }

  std::string RegisterDataModel(std::string agentName, Instance instance) {

    auto result = serviceDirectory_.registerAgent(instance, agentName);
    return std::to_string(result);
  }

  std::string RegisterDataModelTest(PriceTest test) {

    return "test test whoope " + std::to_string(test.price());
  }

  std::vector<std::string> Query(std::string agentName, QueryModel query) {

    /*
    static int test = 0;

    // TODO: (`HUT`) : delete this
    Attribute wind        { "has_wind_speed",   Type::Bool, true};
    Attribute temperature { "has_temperature",  Type::Bool, true};

    ConstraintType eqTrue{ConstraintType::ValueType{Relation{Relation::Op::Eq, true}}};
    Constraint temperature_c { temperature, eqTrue};
    Constraint wind_c        { wind    ,    eqTrue};

    QueryModel query1{{temperature_c}};
    QueryModel query2{{wind_c}}; */

    return serviceDirectory_.query(query);

    //if(test++){
    //  return serviceDirectory_.query(query1);
    //} else {
    //  return serviceDirectory_.query(query2);
    //}
  }

private:
  ServiceDirectory serviceDirectory_;

};


// Next we make a protocol for the implementation
class ServiceProtocol : public Implementation, public Protocol {
public:

  ServiceProtocol() : Implementation(), Protocol() {

    this->Expose(SLOWFUNCTION,          new CallableClassMember<Implementation, int(int, int)>(this, &Implementation::SlowFunction) );
    this->Expose(ADD,                   new CallableClassMember<Implementation, int(int, int)>(this, &Implementation::Add) );
    this->Expose(GREET,                 new CallableClassMember<Implementation, std::string(std::string)>(this, &Implementation::Greet) );
    this->Expose(REGISTERDATAMODEL,     new CallableClassMember<Implementation, std::string(std::string agentName, Instance)>(this, &Implementation::RegisterDataModel) );
    this->Expose(REGISTERDATAMODELTEST, new CallableClassMember<Implementation, std::string(PriceTest)>(this, &Implementation::RegisterDataModelTest) ); // TODO: (`HUT`) : delete this
    this->Expose(QUERY,                 new CallableClassMember<Implementation, std::vector<std::string>(std::string agentName, QueryModel query)>(this, &Implementation::Query) );
  }
};


// And finanly we build the service
class MyCoolService : public ServiceServer< fetch::network::TCPServer > {
public:
  MyCoolService(uint16_t port, fetch::network::ThreadManager *tm) : ServiceServer(port, tm) {
    this->Add(MYPROTO, new ServiceProtocol() );
  }
};


int main() {
  fetch::network::ThreadManager tm(8);
  MyCoolService serv(8080, &tm);
  tm.Start();

  std::string dummy;
  std::cout << "Press ENTER to quit" << std::endl;
  std::cin >> dummy;

  tm.Stop();

  return 0;

}
