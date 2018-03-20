// This file holds and manages connections to other nodes

#ifndef NODE_DIRECTORY_HPP
#define NODE_DIRECTORY_HPP

#include"oef/schema.hpp"
#include"oef/message_history.hpp"
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

  NodeDirectory(network::ThreadManager *tm, const schema::Instance &instance, const schema::Endpoint &nodeEndpoint, const schema::Endpoints &endpoints) :
    tm_{tm},
    instance_{instance},
    endpoints_{endpoints},
    nodeEndpoint_{nodeEndpoint} {}

  ~NodeDirectory() {
    fetch::logger.Info("Destroying NodeDirectory");
    for(auto &i : serviceClients_) {
      delete i.second;
    }
  }

  NodeDirectory& operator=(NodeDirectory& rhs) = delete;
  NodeDirectory& operator=(NodeDirectory&& rhs) = delete;
  //NodeDirectory& operator=(NodeDirectory&& rhs)
  //{
  //     /*instance_       = std::move(rhs.instance());
  //     endpoints_      = std::move(rhs.endpoints());
  //     debugEndpoints_ = std::move(rhs.debugEndpoints_());*/
  //  std::swap(*this, rhs);
  //  return *this;
  //}

  void Start() {

    AddEndpoint(nodeEndpoint_, instance_, endpoints_);
    CallEndpoints(protocols::NodeToNodeRPC::DBG_ADD_ENDPOINT, nodeEndpoint_, instance_, endpoints_);

    // TODO: (`HUT`) : delete this
    /* 
    // On construction, let all of our endpoints know we have arrived
    for(auto &i : endpoints_.endpoints()) {

      // Check we are not connecting to ourself
      if(i.equals(nodeEndpoint_)) {
        continue;
      }

      auto client = GetClient<network::TCPClient>(i);

      fetch::logger.Info(nodeEndpoint_.IP(), ":" ,nodeEndpoint_.TCPPort() ," connecting to:: " ,i.IP() ,":" ,i.TCPPort());

      // Ping them first to check they are there
      if(!CanConnect(client)) {
        fetch::logger.Info("Failed to ping: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",i.IP(),":",i.TCPPort());
      } else {
        fetch::logger.Info("Successfully pinged: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",i.IP(),":",i.TCPPort());
      }

      // Let them know they can add us as an endpoint
      client->Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::DBG_ADD_ENDPOINT, nodeEndpoint_, instance_, endpoints_);
    } */
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

        fetch::network::ThreadManager tm; // TODO: (`HUT`) : use local thread manager when bug is resolved
        service::ServiceClient< fetch::network::TCPClient > client(i.IP(), i.TCPPort(), &tm);
        tm.Start();


      // Ping them first to check they are there
      auto resp = client.Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::PING);

      if(!resp.Wait(pingTimeoutMs_)){
        std::cerr << "no response from node!" << std::endl;
        tm.Stop();
        continue;
      } else {
        std::cerr << "response from node!" << std::endl << std::endl;
      }

      resp  = client.Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::GET_INSTANCE);

      if(!resp.Wait(timeoutMs_)){
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

      if(!resp.Wait(timeoutMs_)){
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

  // Policy: debugEndpoints will start out empty. Other nodes will add themselves to all connections. Nodes hearing this for the first time will forward to their connections
  void AddEndpoint(const schema::Endpoint &endpoint, const schema::Instance &instance, const schema::Endpoints &endpoints) {

    // If we already know of this, do nothing
    if(debugEndpoints_.find(endpoint) != debugEndpoints_.end()){
      return;
    }

    // Let the Node know our details // TODO: (`HUT`) : use common call for this
    std::lock_guard< fetch::mutex::Mutex > lock(serviceClientsMutex_);
    auto client = GetClient<network::TCPClient>(endpoint);
    client->Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::DBG_ADD_ENDPOINT, nodeEndpoint_, instance_, endpoints_);

    // otherwise forward to all known endpoints
    for(auto &i : debugEndpoints_){

      schema::Endpoint forwardTo = i.first;
      client = GetClient<network::TCPClient>(forwardTo);

      if(!CanConnect(client)) {
        fetch::logger.Info("Failed to ping: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      } else {
        fetch::logger.Info("Successfully pinged: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      }

      fetch::logger.Info("Forwarding from:",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort(), " endpoint ", endpoint.IP(), ":", endpoint.TCPPort());
      client->Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::DBG_ADD_ENDPOINT, endpoint, instance, endpoints);
    }

    debugEndpoints_[endpoint] = std::make_pair(instance, endpoints);
  }

  // Info functionality
  script::Variant DebugAllNodes() {
    script::Variant result = script::Variant::Object();

    result["response"] = "success";

    fetch::script::Variant res = fetch::script::Variant::Array(debugEndpoints_.size());

    int index = 0;
    for(auto &i : debugEndpoints_){

      fetch::script::Variant temp = fetch::script::Variant::Object();
      temp["endpoint"] = i.first.variant();
      temp["instance"]  = i.second.first.variant();
      temp["connections"] = i.second.second.variant();
      res[index++] = temp;
    }

    result["value"]   = res;

    return result;
  }

  script::Variant DebugAllEndpoints() {
    script::Variant result = script::Variant::Object();

    result["response"] = "success";

    fetch::script::Variant res = fetch::script::Variant::Array(debugEndpoints_.size());

    int index = 0;
    for(auto &i : debugEndpoints_){

      fetch::script::Variant temp = fetch::script::Variant::Object();
      temp["endpoint"] = i.first.variant();
      res[index++] = temp;
    }

    result["value"]   = res;

    return result;
  }

  // this can be called async. by other nodes
  void addAgent(const schema::Endpoint &endpoint, const std::string &agent) {
    std::lock_guard< fetch::mutex::Mutex > lock(debugAgentsMutex_);
    debugAgents_[endpoint].Insert(agent);
  }

  // this can be called async. by other nodes
  void removeAgent(const schema::Endpoint endpoint, const std::string &agent) {
    std::lock_guard< fetch::mutex::Mutex > lock(debugAgentsMutex_);
    debugAgents_[endpoint].Erase(agent);
  }

  // Registering and deregistering agents
  void RegisterAgent(const std::string &agent) {
    addAgent(nodeEndpoint_, agent);

    // Notify all other endpoints
    CallAllEndpoints(protocols::NodeToNodeRPC::DBG_ADD_AGENT, nodeEndpoint_, agent);
  }

  void DeregisterAgent(const std::string &agent) {
    removeAgent(nodeEndpoint_, agent);

    // Notify all other endpoints
  }

  script::Variant DebugAllAgents() {
    std::lock_guard< fetch::mutex::Mutex > lock(debugAgentsMutex_);
    script::Variant result = script::Variant::Object();

    result["response"] = "success";

    fetch::script::Variant res = fetch::script::Variant::Array(debugAgents_.size());

    int index = 0;
    for(auto &i : debugAgents_){

      fetch::script::Variant temp = fetch::script::Variant::Object();
      temp["endpoint"]            = i.first.variant();
      temp["agents"]              = i.second.variant();
      res[index++]                = temp;
    }

    result["value"]   = res;

    return result;
  }

  void logEvent(const schema::Endpoint &endpoint, const Event &event) {
    std::lock_guard< fetch::mutex::Mutex > lock(debugEventsMutex_);
    std::cout << "hit this add1" << std::endl;
    debugEvents_[endpoint].Insert(event);
  }

  // Query has hit our node
  template <typename T>
  void LogEvent(const std::string source, const T &eventParam) {
    Event event{source, schema::vtos(nodeEndpoint_.variant()), schema::vtos(eventParam.variant())};
    logEvent(nodeEndpoint_, event);

    std::cout << "adding more!" << std::endl << std::endl;

    // Notify all other endpoints
    CallAllEndpoints(protocols::NodeToNodeRPC::DBG_LOG_EVENT, nodeEndpoint_, event);
  }

  script::Variant DebugAllEvents(int maxNumber) {
    std::lock_guard< fetch::mutex::Mutex > lock(debugEventsMutex_);
    script::Variant result = script::Variant::Object();

    result["response"] = "success";

    fetch::script::Variant res = fetch::script::Variant::Array(debugEvents_.size());

    int index = 0;
    for(auto &i : debugEvents_){

      fetch::script::Variant temp = fetch::script::Variant::Object();
      temp["endpoint"]            = i.first.variant();
      temp["events"]              = i.second.variant(maxNumber);
      res[index++]                = temp;
    }

    result["value"]   = res;

    return result;
  }

  script::Variant DebugConnections() {
    script::Variant result = script::Variant::Object();

    result["response"] = "success";
    result["value"]    = endpoints_.variant();

    return result;
  }

  script::Variant DebugEndpoint() {
    script::Variant result = script::Variant::Object();

    result["response"] = "success";
    result["value"]    = nodeEndpoint_.variant();

    return result;
  }

  // Helper functions
  template <typename T>
  bool CanConnect(T &client) {

    for (int i = 0; i < 5; ++i) {
      auto resp = client->Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::PING);

      if(resp.Wait(pingTimeoutMs_)){
        return true;
      }
    }
    return false;
  }

  // Call the endpoints that we know of
  template<typename T, typename... Args>
  void CallEndpoints(T CallEnum, Args... args) {

    for(auto &forwardTo : endpoints_.endpoints()){

      // Check we are not connecting to ourself (we have a lock on this directory)
      if(forwardTo.equals(nodeEndpoint_)) {
        fetch::logger.Info("=== do not forward to ourself");
        continue;
      }
      fetch::logger.Info("=== we got here3!!");

      std::lock_guard< fetch::mutex::Mutex > lock(serviceClientsMutex_);
      auto client = GetClient<network::TCPClient>(forwardTo);

      fetch::logger.Info("===Sending to endpoint",  forwardTo.IP(),":",forwardTo.TCPPort());

      // Ping them first to check they are there
      if(!CanConnect(client)) {
        fetch::logger.Info("Failed to ping: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      } else {
        fetch::logger.Info("Successfully pinged: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      }

      client->Call( protocols::FetchProtocols::NODE_TO_NODE, CallEnum, args...);
    }
  }

  // Call all the debug endpoints that we know of
  template<typename T, typename... Args>
  void CallAllEndpoints(T CallEnum, Args... args) {

    fetch::logger.Info("****************** we got here1!!", endpoints_.endpoints().size());

    for(auto &i : debugEndpoints_){

      schema::Endpoint forwardTo = i.first;

      // Check we are not connecting to ourself (we have a lock on this directory)
      if(forwardTo.equals(nodeEndpoint_)) {
        fetch::logger.Info("****************** do not forward to ourself");
        continue;
      }
      fetch::logger.Info("****************** we got here3!!");

      std::lock_guard< fetch::mutex::Mutex > lock(serviceClientsMutex_);
      auto client = GetClient<network::TCPClient>(forwardTo);

      fetch::logger.Info("******************Sending to endpoint",  forwardTo.IP(),":",forwardTo.TCPPort());

      // Ping them first to check they are there
      if(!CanConnect(client)) {
        fetch::logger.Info("Failed to ping: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      } else {
        fetch::logger.Info("Successfully pinged: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      }

      client->Call( protocols::FetchProtocols::NODE_TO_NODE, CallEnum, args...);
    }
  }

  template <typename T>
  service::ServiceClient<T> *GetClient(schema::Endpoint endpoint) {

    if(serviceClients_.find(endpoint) == serviceClients_.end()){
      serviceClients_[endpoint] = new service::ServiceClient<T> {endpoint.IP(), endpoint.TCPPort(), tm_};
    }

    return serviceClients_[endpoint];
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // Debug functions


  schema::Instance  &instance()                                                                { return instance_; }
  schema::Endpoints &endpoints()                                                               { return endpoints_; }
  //std::map<schema::Endpoint, std::pair<schema::Instance, schema::Endpoints>> &debugEndpoints() { return debugEndpoints_; }

private:
  fetch::network::ThreadManager *tm_;
  schema::Instance              instance_;
  schema::Endpoints             endpoints_;
  const double                  timeoutMs_     = 9000;
  const double                  pingTimeoutMs_ = 1000; // Important that this is << than timeoutMs

  // debug functionality
  schema::Endpoint                                                           nodeEndpoint_;
  std::map<schema::Endpoint, std::pair<schema::Instance, schema::Endpoints>> debugEndpoints_;
  std::map<schema::Endpoint, schema::Agents>                                 debugAgents_;
  fetch::mutex::Mutex                                                        debugAgentsMutex_;

  std::map<schema::Endpoint, Events>                                         debugEvents_;
  fetch::mutex::Mutex                                                        debugEventsMutex_;

  // constantly active service clients
  std::map<schema::Endpoint, service::ServiceClient<fetch::network::TCPClient> *> serviceClients_;
  fetch::mutex::Mutex                                                             serviceClientsMutex_;

};

}
}
#endif

