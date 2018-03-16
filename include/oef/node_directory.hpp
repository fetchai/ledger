// This file holds and manages connections to other nodes

#ifndef NODE_DIRECTORY_HPP
#define NODE_DIRECTORY_HPP

#include"oef/schema.hpp"
#include"service/client.hpp"
#include"protocols/fetch_protocols.hpp"
#include"protocols/node_to_node/commands.hpp"

namespace fetch
{
namespace oef
{

class NodeDirectory {
public:
  explicit NodeDirectory() = default;

  NodeDirectory(const schema::Instance &instance, const schema::Endpoints &endpoints) :
    instance_{instance},
    endpoints_{endpoints} { }

  void Start() {
    tm_.Start();
  }

  ~NodeDirectory() {
    tm_.Stop();
  }

  NodeDirectory& operator=(NodeDirectory&& rhs)
  {
       instance_  = std::move(rhs.instance());
       endpoints_ = std::move(rhs.endpoints());
       return *this;
  }

  schema::Instance getInstance() {
    return instance_;
  }

  std::vector<std::string> Query(const schema::QueryModelMulti &query) {

    std::vector<std::string> response;
    if(query.jumps() == 0) { return response; }

    std::ostringstream test; // TODO: (`HUT`) : remove this
    test << instance_.variant();
    std::cout << "our instance is " << test.str() << std::endl;

    for(auto &i : endpoints_.endpoints()) {
      std::cerr << "checking against " << i.IP() << ":" << i.TCPPort() << std::endl;

        fetch::network::ThreadManager tm;
        service::ServiceClient< fetch::network::TCPClient > client(i.IP(), i.TCPPort(), &tm); // TODO: (`HUT`) : use local thread manager when bug is resolved
        tm.Start();

      try {
        std::cerr << "constructed thing" << i.IP() << ":" << i.TCPPort() << std::endl;

        // Ping them first to check they are there
        auto resp = client.Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::PING);

        std::cerr << "call thing" << i.IP() << ":" << i.TCPPort() << std::endl;

        if(!resp.Wait(pingTimeoutMs)){
          std::cerr << "no response from node!" << std::endl;
          tm.Stop();
          continue;
        } else {
          std::cerr << "response from node!" << std::endl << std::endl;
        }

      } catch (...) {
          std::cerr << "node doesn't appear to exist!" << std::endl << std::endl;
      }

      auto resp  = client.Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::GET_INSTANCE);

      if(!resp.Wait(timeoutMs)){
        std::cerr << "no response from node when getting instance!" << std::endl;
        tm.Stop();
        continue;
      }

      schema::Instance instance = resp.As<schema::Instance>();
      /*
      schema::QueryModel aaa = query.forwardingQuery();
      std::ostringstream find; // TODO: (`HUT`) : remove this
      find << instance.variant();
      std::cerr << "their instance is " << find.str() << std::endl;
      std::cerr << "our constraint is " << aaa.variant() << std::endl;

      std::cerr << "datamodel name is" << instance.model().name() << std::endl;

      schema::ConstraintType  lessThan300 {schema::ConstraintType::ValueType{schema::Relation{schema::Relation::Op::Gt, 2}}};
      schema::Attribute       longitude   { "longitude",        schema::Type::Int, true};
      schema::Constraint      longitude_c   { longitude    ,    lessThan300};
      schema::QueryModel      bbb{{longitude_c}};

      std::vector<schema::Attribute> attributes{longitude};
      schema::DataModel weather{"testthis", attributes};
      schema::Instance arghee{weather, {{"longitude", "100"}}};
      schema::Instance arghbb{weather, {{"longitude", "100"}}};

      std::cout << "begin the comparing" << std::endl;

      if(aaa.check(arghee)) { std::cout << "match 1" << std::endl; }

      arghee.dataModel() = instance.dataModel();

      if(aaa.check(arghee)) { std::cout << "match 2" << std::endl; }

      if(aaa.check(arghbb)) { std::cout << "match 3" << std::endl; }

      arghbb.values() = instance.values();

      if(aaa.check(arghbb)) { std::cout << "match 4" << std::endl; }

      if(aaa.check(instance)) { std::cout << "match 5" << std::endl; } */

      if(query.forwardingQuery().check(instance)) {
        std::cerr << "Forwarding match!" << std::endl;
      } else {
        std::cerr << "Forwarding fail!" << std::endl;
        tm.Stop();
        continue;
      }

      auto nextQuery = query;
      nextQuery--;

      resp  = client.Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::QUERY, nextQuery);

      if(!resp.Wait(timeoutMs)){
        std::cerr << "no response from node when getting forwarded query!" << std::endl;
        tm.Stop();
        continue;
      }

      auto multiResult = resp.As<std::vector<std::string>>();

      response.insert(response.end(), multiResult.begin(), multiResult.end());

      tm.Stop();
    }

    return response;
  }

  schema::Instance  &instance() { return instance_; }
  schema::Endpoints &endpoints() { return endpoints_; }

private:
  schema::Instance              instance_;
  schema::Endpoints             endpoints_;
  fetch::network::ThreadManager tm_;
  const double timeoutMs     = 500;
  const double pingTimeoutMs = 5;
};

}
}
#endif

