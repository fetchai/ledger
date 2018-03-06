#ifndef NODE_OEF_HPP
#define NODE_OEF_HPP

#include<iostream>
#include"service/server.hpp"
#include"oef/service_consts.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"oef/service_directory.hpp"

// Core OEF implementation
class NodeOEF {

public:
  std::string RegisterInstance(std::string agentName, Instance instance) {
    auto result = serviceDirectory_.RegisterAgent(instance, agentName);

    fetch::logger.Info("Registering instance: ", instance.getDataModel().getName(), " by AEA: ", agentName);
    return std::to_string(result);
  }

  std::vector<std::string> Query(QueryModel query) {
    return serviceDirectory_.Query(query);
  }

  std::string test() {
    return std::string{"this is a test"};
  }

private:
  service_directory serviceDirectory_;
};

#endif
