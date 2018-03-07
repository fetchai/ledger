#ifndef NODE_OEF_HPP
#define NODE_OEF_HPP

#include<iostream>
#include"service/server.hpp"
#include"oef/service_consts.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"oef/service_directory.hpp"

namespace fetch
{
namespace node_oef
{

// Core OEF implementation
class NodeOEF {

public:
  std::string RegisterInstance(std::string agentName, schema::Instance instance) {
    auto result = serviceDirectory_.RegisterAgent(instance, agentName);

    fetch::logger.Info("Registering instance: ", instance.dataModel().name(), " by AEA: ", agentName);
    return std::to_string(result);
  }

  std::vector<std::string> Query(schema::QueryModel query) {
    return serviceDirectory_.Query(query);
  }

  std::string test() {
    return std::string{"this is a test"};
  }

private:
  service_directory::service_directory serviceDirectory_;
};

}
}

#endif
