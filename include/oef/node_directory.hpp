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
    nodeEndpoint_{nodeEndpoint} {
      nodeName_ = instance_.values()["name"]; // TODO: (`HUT`) : this relies on this existing
      fetch::logger.Info("Constructed NodeDirectory");
    }

  ~NodeDirectory() {
    fetch::logger.Info("Destroying NodeDirectory");
    for(auto &i : serviceClients_) {
      delete i.second;
    }
  }

  NodeDirectory& operator=(NodeDirectory& rhs) = delete;
  NodeDirectory& operator=(NodeDirectory&& rhs) = delete;

  void Start() {
    fetch::logger.Info("Starting NodeDirectory");
    AddEndpoint(nodeEndpoint_, instance_, endpoints_);
    CallEndpoints(protocols::NodeToNodeRPC::DBG_ADD_ENDPOINT, true, nodeEndpoint_, instance_, endpoints_);
  }

  schema::Instance getInstance() {
    return instance_;
  }

  bool shouldForward(schema::QueryModelMulti queryMulti) {
    return queryMulti.jumps() > 0 && queryMulti.forwardingQuery().check(instance_);
  }

  // TODO: (`HUT`) : delete this function and associated code
  /*
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
  } */

  // Policy: debugEndpoints will start out empty. Other nodes will add themselves to all connections. Nodes hearing this for the first time will forward to their connections
  void AddEndpoint(const schema::Endpoint &endpoint, const schema::Instance &instance, const schema::Endpoints &endpoints) {

    // If we already know of this, do nothing
    if(debugEndpoints_.find(endpoint) != debugEndpoints_.end()){
      return;
    }

    // TODO: (`HUT`) : ****** CHANGE ******
    // TODO: (`HUT`) : not like this
    // Let the ORIGINAL Node know our details // TODO: (`HUT`) : use common call for this
    serviceClientsMutex_.lock();
    auto client = GetClient<network::TCPClient>(endpoint);
    client->Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::DBG_ADD_ENDPOINT, nodeEndpoint_, instance_, endpoints_);
    serviceClientsMutex_.unlock();

    // otherwise forward to all known endpoints
    for(auto &i : debugEndpoints_){

      schema::Endpoint forwardTo = i.first;

      if(!CanConnect(forwardTo)) {
        fetch::logger.Info("Failed to ping: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      } else {
        fetch::logger.Info("Successfully pinged: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      }

      std::lock_guard< fetch::mutex::Mutex > lock(serviceClientsMutex_);
      client = GetClient<network::TCPClient>(forwardTo);

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
    debugEvents_[endpoint].Insert(event);
  }

  // By AEA
  void ForwardQuery(const schema::QueryModelMulti &queryModel) {

    if(queryModel.jumps() == 0) {
      return;
    }

    auto query = queryModel;
    query--; // reduce jumps by 1

    fetch::logger.Info("Forwarding query to endpoints");
    CallEndpoints(protocols::NodeToNodeRPC::FORWARD_QUERY, false, nodeName_, nodeEndpoint_, query);
    fetch::logger.Info("Finished forwarding query to endpoints");
  }

  // By Node
  void ForwardQuery(const schema::Endpoint &endpoint, const schema::QueryModelMulti &queryModel) {

    if(queryModel.jumps() == 0 || !shouldForward(queryModel)) {
      return;
    }

    auto query = queryModel;
    query--; // reduce jumps by 1

    // Set up a return path for query answers
    messageBoxesMutex_.lock();
    messageBoxCallback_[queryModel] = endpoint;
    messageBoxesMutex_.unlock();

    CallEndpoints(protocols::NodeToNodeRPC::FORWARD_QUERY, false, nodeName_, nodeEndpoint_, query);
  }

  void ReturnQuery(const schema::QueryModelMulti &queryModel, const std::vector<std::string> &agents) {
    // If we have a return path set up, use that, otherwise dump it in the message box
    messageBoxesMutex_.lock();
    if(messageBoxCallback_.find(queryModel) != messageBoxCallback_.end()){
      std::cout << "Forwarding return query!" << std::endl;
      CallEndpoint(protocols::NodeToNodeRPC::RETURN_QUERY, messageBoxCallback_[queryModel], queryModel, agents);
      messageBoxesMutex_.unlock();
      return;
    }

    std::cout << "Received return query! Adding to " << schema::vtos(queryModel.variant()) << std::endl;
    for(auto &i : agents) {
      std::cout << i << std::endl;
    }

    //auto result = messageBox_[queryModel];
    messageBox_[queryModel] = agents; // TODO: (`HUT`) : this does not collate results

    //result.insert(result.end(), agents.begin(), agents.end());
    messageBoxesMutex_.unlock();
  }

  // Get results (TODO: (`HUT`) : and clean message box)
  std::vector<std::string> &ForwardQueryResult(const schema::QueryModelMulti &queryModel) {
    std::lock_guard< fetch::mutex::Mutex > lock(messageBoxesMutex_);

    std::cout << "checking return query! " << schema::vtos(queryModel.variant()) << std::endl;

    for(auto &i : messageBox_[queryModel]) {
      std::cout << "WHOPPEE" << i << std::endl;
    }

    return messageBox_[queryModel];
  }

  // Query has hit our node
  template <typename T>
  void LogEvent(const std::string source, const T &eventParam) {
    Event event{source, nodeName_, schema::vtos(eventParam.variant())};
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
  bool CanConnect(T &endpoint) {

    for (int i = 0; i < 20; ++i) {

      serviceClientsMutex_.lock();
      auto client = GetClient<network::TCPClient>(endpoint);

      auto resp = client->Call( protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::PING);
      serviceClientsMutex_.unlock();

      if(resp.Wait(pingTimeoutMs_)){
        return true;
      }

      uint64_t waitTime = 5 + (static_cast<uint64_t>(time(NULL)) % 10);
      std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
    }
    return false;
  }

  // non blocking call endpoint
  template<typename T, typename... Args>
  void CallEndpoint(T CallEnum, schema::Endpoint endpoint, Args... args) {
    std::lock_guard< fetch::mutex::Mutex > lock(serviceClientsMutex_);
    auto client = GetClient<network::TCPClient>(endpoint);
    client->Call( protocols::FetchProtocols::NODE_TO_NODE, CallEnum, args...);
  }

  // Call the endpoints that we know of
  template<typename T, typename... Args>
  void CallEndpoints(T CallEnum, bool pingFirst, Args... args) {

    for(auto &forwardTo : endpoints_.endpoints()){

      // Check we are not connecting to ourself (we have a lock on this directory)
      if(forwardTo.equals(nodeEndpoint_)) {
        continue;
      }

      // Ping them first to check they are there
      if(pingFirst) {
        if(!CanConnect(forwardTo)) {
          fetch::logger.Info("Failed to ping: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
        } else {
          fetch::logger.Info("Successfully pinged: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
        }
      }

      std::lock_guard< fetch::mutex::Mutex > lock(serviceClientsMutex_);
      auto client = GetClient<network::TCPClient>(forwardTo);
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

      // Ping them first to check they are there
      if(!CanConnect(forwardTo)) {
        fetch::logger.Info("Failed to ping: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      } else {
        fetch::logger.Info("Successfully pinged: ",  nodeEndpoint_.IP(),":",nodeEndpoint_.TCPPort()," to ",forwardTo.IP(),":",forwardTo.TCPPort());
      }

      std::lock_guard< fetch::mutex::Mutex > lock(serviceClientsMutex_);
      auto client = GetClient<network::TCPClient>(forwardTo);
      client->Call( protocols::FetchProtocols::NODE_TO_NODE, CallEnum, args...);
    }
  }

  // TODO: (`HUT`) : this is not elegant
  template <typename T>
  service::ServiceClient<T> *GetClient(schema::Endpoint endpoint) {

    if(serviceClients_.find(endpoint) == serviceClients_.end()){
      serviceClients_[endpoint] = new service::ServiceClient<T> {endpoint.IP(), endpoint.TCPPort(), tm_};
    }

    return serviceClients_[endpoint];
    //return std::make_shared<service::ServiceClient<T>>(service::ServiceClient<T> {endpoint.IP(), endpoint.TCPPort(), tm_});
  }

  //////////////////////////////////////////////////////////////////////////////////////////
  // Debug functions


  schema::Instance  &instance()                                                                { return instance_; }
  schema::Endpoints &endpoints()                                                               { return endpoints_; }
  //std::map<schema::Endpoint, std::pair<schema::Instance, schema::Endpoints>> &debugEndpoints() { return debugEndpoints_; }

private:
  fetch::network::ThreadManager *tm_;
  std::string                   nodeName_;
  schema::Instance              instance_;
  schema::Endpoints             endpoints_;
  const double                  timeoutMs_     = 9000;
  const double                  pingTimeoutMs_ = 500; // Important that this is << than timeoutMs

  // Message box functionality, TODO: (`HUT`) : make this its own class
  std::map<schema::QueryModelMulti, std::vector<std::string>> messageBox_;
  std::map<schema::QueryModelMulti, schema::Endpoint>         messageBoxCallback_;
  fetch::mutex::Mutex                                         messageBoxesMutex_;

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

