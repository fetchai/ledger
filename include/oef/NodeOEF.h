#ifndef NODE_OEF_H
#define NODE_OEF_H

#include<iostream>
#include"service/server.hpp"
#include"oef/service_consts.hpp"
#include"oef/schema.h"
#include"oef/schemaSerializers.h"
#include"oef/ServiceDirectory.h"

class NodeOEF {

public:
  std::string RegisterDataModel(std::string agentName, Instance instance) {
    auto result = serviceDirectory_.RegisterAgent(instance, agentName);
    return std::to_string(result);
  }

  std::vector<std::string> Query(std::string agentName, QueryModel query) {
    return serviceDirectory_.Query(query);
  }

  std::string test() {
    std::cout << "hit the test condition" << std::endl;
    return std::string{"this is a test"};
  }

private:
  ServiceDirectory serviceDirectory_;
};

#endif
