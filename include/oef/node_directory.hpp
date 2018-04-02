#ifndef NODE_DIRECTORY_HPP
#define NODE_DIRECTORY_HPP

#include<math.h>
#include<map>
#include<utility>
#include<string>
#include<vector>
#include<set>
#include"oef/schema.hpp"
#include"oef/message_history.hpp"
#include"service/client.hpp"
#include"protocols/fetch_protocols.hpp"
#include"protocols/node_to_node/commands.hpp"

// This file holds and manages connections to other nodes

namespace fetch
{
namespace oef
{

class NodeDirectory {
public:
  NodeDirectory() = default;

  NodeDirectory(network::ThreadManager *tm, const schema::Instance &instance, const schema::Endpoint &nodeEndpoint, const schema::Endpoints &endpoints) :
    tm_{tm},
    instance_{instance},
    endpoints_{endpoints},
    nodeEndpoint_{nodeEndpoint} {
      nodeName_ = instance_.values()["name"]; // TODO: (`HUT`) : bad - this relies on this attribute existing
      fetch::logger.Info("Constructed NodeDirectory");
    }

  ~NodeDirectory() {
    fetch::logger.Info("Destroying NodeDirectory");

    std::lock_guard< fetch::mutex::Mutex > lock(serviceClientsMutex_);
    for (auto &i : serviceClients_) {
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

  void setInstance(schema::Instance instance) {
      nodeName_ = instance.values()["name"]; // TODO: (`HUT`) : this relies on this existing
      instance_ = instance;

      // Update all nodes with this new info
      debugEndpoints_[nodeEndpoint_].first = instance;
      CallAllEndpoints(protocols::NodeToNodeRPC::DBG_UPDATE_ENDPOINT, nodeEndpoint_, instance_);
  }

  // Check if any packets we see should be forwarded
  bool shouldForward(schema::QueryModelMulti queryMulti) {

    if (!(queryMulti.jumps() > 0)) {
      return false;
    }

    const schema::QueryModel &fwd = queryMulti.forwardingQuery();

    // catch special directional search
    if (fwd.angle1() != 0 || fwd.angle2() != 0) {

      if (fwd.lat().compare(instance_.values()["latitude"]) == 0 && fwd.lng().compare(instance_.values()["longitude"]) == 0) {
        fetch::logger.Info("Forwarding parameter matches our lat/long");
      } else {

        float ourLat;
        float ourLng;
        float originLat;
        float originLng;
        float angle1 = fwd.angle1();
        float angle2 = fwd.angle2();

        std::istringstream buffer(instance_.values()["latitude"]);
        buffer >> ourLat;

        buffer = std::istringstream(instance_.values()["longitude"]);
        buffer >> ourLng;

        buffer = std::istringstream(fwd.lat());
        buffer >> originLat;

        buffer = std::istringstream(fwd.lng());
        buffer >> originLng;

        float ourAngleToOrigin = atan2((ourLng - originLng), ((ourLat - originLat)));

        ourAngleToOrigin += (M_PI*2);

        while (ourAngleToOrigin > (M_PI*2)) {
          ourAngleToOrigin -= M_PI*2;
        }

        // Avoid modular edge case
        if (angle2 < angle1) {

          if (ourAngleToOrigin >= angle1 || ourAngleToOrigin <= angle2) {
            return true;
          }
        }

        if (ourAngleToOrigin >= angle1 && ourAngleToOrigin <= (angle2)) {
          return true;
        } else {
          return false;
        }
      }
    }

    // original comparison
    return queryMulti.forwardingQuery().check(instance_);
  }

    // get and set the original info
  void UpdateEndpoint(const schema::Endpoint &endpoint, const schema::Instance &instance) {
    debugEndpoints_[endpoint].first = instance;
  }

  void addConnection(const schema::Endpoint &endpoint) {
    mutex_.lock();
    std::set<schema::Endpoint> &ref = endpoints_.endpoints();
    ref.insert(endpoint);
    mutex_.unlock();

    // Update all nodes with this new info TODO: (`HUT`) : not elegant
    std::set<schema::Endpoint> &debugRef  = debugEndpoints_[nodeEndpoint_].second.endpoints();
    debugRef.insert(endpoint);

    CallAllEndpoints(protocols::NodeToNodeRPC::DBG_ADD_CONNECTION, nodeEndpoint_, endpoint);
  }

  void DebugAddConnection(const schema::Endpoint &endpoint, const schema::Endpoint &connection) {
    std::set<schema::Endpoint> &debugRef  = debugEndpoints_[endpoint].second.endpoints();
    debugRef.insert(connection);
  }

  // Policy: debugEndpoints will start out empty. Other nodes will add themselves to all connections. Nodes hearing this for the first time will forward to their connections
  void AddEndpoint(const schema::Endpoint &endpoint, const schema::Instance &instance, const schema::Endpoints &endpoints) {

    // If we already know of this, do nothing
    if (debugEndpoints_.find(endpoint) != debugEndpoints_.end()) {
      return;
    }

    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

    // TODO: (`HUT`) : not like this
    // Let the ORIGINAL Node know our details // TODO: (`HUT`) : use common call for this
    auto client = GetClient<network::TCPClient>(endpoint);
    client->Call(protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::DBG_ADD_ENDPOINT, nodeEndpoint_, instance_, endpoints_);

    // otherwise forward to all known endpoints
    for (auto &i : debugEndpoints_) {

      schema::Endpoint forwardTo = i.first;

      if (!CanConnect(forwardTo)) {
        fetch::logger.Info("Failed to ping: ",  nodeEndpoint_.IP(), ":", nodeEndpoint_.TCPPort(), " to ", forwardTo.IP(), ":", forwardTo.TCPPort());
      } else {
        fetch::logger.Info("Successfully pinged: ", nodeEndpoint_.IP(), ":", nodeEndpoint_.TCPPort(), " to ", forwardTo.IP(), ":", forwardTo.TCPPort());
      }

      client = GetClient<network::TCPClient>(forwardTo);

      fetch::logger.Info("Forwarding from:", nodeEndpoint_.IP(), ":", nodeEndpoint_.TCPPort(), " to ", forwardTo.IP(), ":", forwardTo.TCPPort(), " endpoint ", endpoint.IP(), ":", endpoint.TCPPort());
      client->Call(protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::DBG_ADD_ENDPOINT, endpoint, instance, endpoints);
    }

    debugEndpoints_[endpoint] = std::make_pair(instance, endpoints);
  }

  // Info functionality
  script::Variant DebugAllNodes() {
    script::Variant result = script::Variant::Object();

    result["response"] = "success";

    fetch::script::Variant res = fetch::script::Variant::Array(debugEndpoints_.size());

    int index = 0;
    for (auto &i : debugEndpoints_) {

      fetch::script::Variant temp = fetch::script::Variant::Object();
      temp["endpoint"] = i.first.variant();
      temp["instance"]  = i.second.first.variant();
      temp["connections"] = i.second.second.variant();

      // A hack but get the agent list here
      temp["agents"] = debugAgents_[i.first].variant();

      res[index++] = temp;
    }

    result["value"]   = res;

    return result;
  }

  // TODO: (`HUT`) : Make HTTP interface the only 'success' variant creator
  script::Variant DebugAllEndpoints() {
    script::Variant result = script::Variant::Object();

    result["response"] = "success";

    fetch::script::Variant res = fetch::script::Variant::Array(debugEndpoints_.size());

    int index = 0;
    for (auto &i : debugEndpoints_) {

      fetch::script::Variant temp = fetch::script::Variant::Object();
      temp["endpoint"] = i.first.variant();
      res[index++] = temp;
    }

    result["value"]   = res;

    return result;
  }

  // this can be called async. by other nodes
  void addAgent(const schema::Endpoint &endpoint, const std::string &agent, const schema::Instance instance) {
    std::lock_guard< fetch::mutex::Mutex > lock(debugAgentsMutex_);
    debugAgents_[endpoint].Insert(agent);
    debugAgentsWithInstances_[agent] = instance;
  }

  // this can be called async. by other nodes
  void removeAgent(const schema::Endpoint endpoint, const std::string &agent) {
    std::lock_guard< fetch::mutex::Mutex > lock(debugAgentsMutex_);
    debugAgents_[endpoint].Erase(agent);
    debugAgentsWithInstances_.erase(agent);
  }

  // Registering and deregistering agents
  void RegisterAgent(const std::string &agent, const schema::Instance instance) {
    addAgent(nodeEndpoint_, agent, instance);

    // Notify all other endpoints
    CallAllEndpoints(protocols::NodeToNodeRPC::DBG_ADD_AGENT, nodeEndpoint_, agent, instance);
  }

  void DeregisterAgent(const std::string &agent) {
    removeAgent(nodeEndpoint_, agent);

    // Notify all other endpoints
    CallAllEndpoints(protocols::NodeToNodeRPC::DBG_REMOVE_AGENT, nodeEndpoint_, agent);
  }

  script::Variant DebugAllAgents() {
    std::lock_guard< fetch::mutex::Mutex > lock(debugAgentsMutex_);
    script::Variant result = script::Variant::Object();

    result["response"] = "success";
    fetch::script::Variant res = fetch::script::Variant::Array(debugAgentsWithInstances_.size());

    int index = 0;
    for (auto &i : debugAgentsWithInstances_) {
      res[index++] = i.second.variant();
    }

    result["value"]   = res;

    return result;
  }

  void logEvent(const schema::Endpoint &endpoint, const Event &event) {
    std::lock_guard< fetch::mutex::Mutex > lock(debugEventsMutex_);
    debugEventsNoEndpoint_.Insert(event);
  }

  // By AEA
  void ForwardQuery(const schema::QueryModelMulti &queryModel) {

    if (queryModel.jumps() == 0) {
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

    if (queryModel.jumps() == 0 || !shouldForward(queryModel)) {
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
    if (messageBoxCallback_.find(queryModel) != messageBoxCallback_.end()) {
      std::cout << "Forwarding return query!" << std::endl;
      CallEndpoint(protocols::NodeToNodeRPC::RETURN_QUERY, messageBoxCallback_[queryModel], queryModel, agents);
      messageBoxesMutex_.unlock();

      return;
    }

    std::cout << "Received return query! Adding to " << schema::vtos(queryModel.variant()) << std::endl;
    for (auto &i : agents) {
      std::cout << i << std::endl;
    }

    std::vector<std::string> &result = messageBox_[queryModel];

    result.insert(result.end(), agents.begin(), agents.end());
    messageBoxesMutex_.unlock();
  }

  // Get results (TODO: (`HUT`) : and clean message box)
  std::vector<std::string> &ForwardQueryResult(const schema::QueryModelMulti &queryModel) {
    std::lock_guard< fetch::mutex::Mutex > lock(messageBoxesMutex_);
    return messageBox_[queryModel];
  }

  void ForwardQueryClean(const schema::QueryModelMulti &queryModel) {
    std::lock_guard< fetch::mutex::Mutex > lock(messageBoxesMutex_);
    messageBox_.erase(queryModel);
  }

  // Special for demo TODO: (`HUT`) : refactor
  template <typename T>
  void LogEventReverse(const std::string source, const T &eventParam, bool wasOrigin = false) {

    std::string hash = eventParam.getHash();
    Event event{nodeName_, source, schema::vtos(eventParam.variant()), hash, wasOrigin};
    logEvent(nodeEndpoint_, event);

    std::cout << "adding more!" << std::endl << std::endl;

    // Notify all other endpoints
    CallAllEndpoints(protocols::NodeToNodeRPC::DBG_LOG_EVENT, nodeEndpoint_, event);
  }

  // Query has hit our node
  template <typename T>
  void LogEvent(const std::string source, const T &eventParam, bool wasOrigin = false) {

    std::string hash = eventParam.getHash();
    Event event{source, nodeName_, schema::vtos(eventParam.variant()), hash, wasOrigin};
    logEvent(nodeEndpoint_, event);

    std::cout << "adding more!" << std::endl << std::endl;

    // Notify all other endpoints
    CallAllEndpoints(protocols::NodeToNodeRPC::DBG_LOG_EVENT, nodeEndpoint_, event);
  }

  script::Variant DebugAllEvents(int maxNumber) {
    std::lock_guard< fetch::mutex::Mutex > lock(debugEventsMutex_);

    script::Variant result = script::Variant::Object();

    result["response"] = "success";
    result["value"]    = debugEventsNoEndpoint_.variant(maxNumber);

    return result;
  }

  script::Variant DebugConnections() {
    script::Variant result = script::Variant::Object();

    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
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

      auto client = GetClient<network::TCPClient>(endpoint);

      auto resp = client->Call(protocols::FetchProtocols::NODE_TO_NODE, protocols::NodeToNodeRPC::PING);

      if (resp.Wait(pingTimeoutMs_)) {
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
    auto client = GetClient<network::TCPClient>(endpoint);
    client->Call(protocols::FetchProtocols::NODE_TO_NODE, CallEnum, args...);
  }

  // Call the endpoints that we know of
  template<typename T, typename... Args>
  void CallEndpoints(T CallEnum, bool pingFirst, Args... args) {

    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

    for (auto &forwardTo : endpoints_.endpoints()) {

      // Check we are not connecting to ourself (we have a lock on this directory)
      if (forwardTo.equals(nodeEndpoint_)) {
        continue;
      }

      // Ping them first to check they are there
      if (pingFirst) {
        if (!CanConnect(forwardTo)) {
          fetch::logger.Info("Failed to ping: ",  nodeEndpoint_.IP(), ":", nodeEndpoint_.TCPPort(), " to ", forwardTo.IP(), ":", forwardTo.TCPPort());
        } else {
          fetch::logger.Info("Successfully pinged: ",  nodeEndpoint_.IP(), ":", nodeEndpoint_.TCPPort(), " to ", forwardTo.IP(), ":", forwardTo.TCPPort());
        }
      }

      auto client = GetClient<network::TCPClient>(forwardTo);
      client->Call(protocols::FetchProtocols::NODE_TO_NODE, CallEnum, args...);
    }
  }

  // Call all the debug endpoints that we know of
  template<typename T, typename... Args>
  void CallAllEndpoints(T CallEnum, Args... args) {

    for (auto &i : debugEndpoints_) {

      schema::Endpoint forwardTo = i.first;

      // Check we are not connecting to ourself (we have a lock on this directory)
      if (forwardTo.equals(nodeEndpoint_)) {
        continue;
      }

      auto client = GetClient<network::TCPClient>(forwardTo);
      client->Call(protocols::FetchProtocols::NODE_TO_NODE, CallEnum, args...);
    }
  }

  // TODO: (`HUT`) : use shared ptrs for service clients
  template <typename T>
  service::ServiceClient<T> *GetClient(schema::Endpoint endpoint) {

    if (serviceClients_.find(endpoint) == serviceClients_.end()) {
      serviceClients_[endpoint] = new service::ServiceClient<T> {endpoint.IP(), endpoint.TCPPort(), tm_};
    }

    return serviceClients_[endpoint];
  }

  schema::Instance  &instance()  { return instance_; }
  schema::Endpoints &endpoints() { std::lock_guard< fetch::mutex::Mutex > lock(mutex_); return endpoints_; }

private:
  fetch::network::ThreadManager *tm_;
  std::string                   nodeName_;
  schema::Instance              instance_;
  schema::Endpoints             endpoints_;
  const double                  timeoutMs_     = 9000;
  const double                  pingTimeoutMs_ = 500; // Important that this is << than timeoutMs
  fetch::mutex::Mutex           mutex_;

  // Message box functionality, TODO: (`HUT`) : make this its own class
  std::map<schema::QueryModelMulti, std::vector<std::string>> messageBox_;
  std::map<schema::QueryModelMulti, schema::Endpoint>         messageBoxCallback_;
  fetch::mutex::Mutex                                         messageBoxesMutex_;

  // debug functionality
  schema::Endpoint                                                           nodeEndpoint_;
  std::map<schema::Endpoint, std::pair<schema::Instance, schema::Endpoints>> debugEndpoints_;
  std::map<schema::Endpoint, schema::Agents>                                 debugAgents_;
  std::map<std::string, schema::Instance>                                    debugAgentsWithInstances_;
  fetch::mutex::Mutex                                                        debugAgentsMutex_;

  Events                                                                     debugEventsNoEndpoint_;
  std::map<schema::Endpoint, Events>                                         debugEvents_;
  fetch::mutex::Mutex                                                        debugEventsMutex_;

  // constantly active service clients
  std::map<schema::Endpoint, service::ServiceClient<fetch::network::TCPClient> *> serviceClients_;
  fetch::mutex::Mutex                                                             serviceClientsMutex_;
};

}
}
#endif

