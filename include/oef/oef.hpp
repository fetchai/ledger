#ifndef OEF_HPP
#define OEF_HPP

#include<iostream>
#include<set>
#include<map>
#include<string>
#include<fstream>
#include<algorithm>
#include<utility>
#include<vector>
#include"service/server.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"oef/service_directory.hpp"
#include"oef/node_directory.hpp"
#include"oef/aea_directory.hpp"
#include"oef/message_history.hpp"
#include"protocols/fetch_protocols.hpp"
#include"protocols/node_to_aea/commands.hpp"

// This represents the API to the OEF: all HTTP and RPC OEF commands go through here

namespace fetch
{
namespace oef
{

struct Transaction {
  int64_t               amount;
  byte_array::ByteArray fromAddress;
  byte_array::ByteArray notes;
  uint64_t              time;
  byte_array::ByteArray toAddress;
  script::Variant       json;
};

// TODO: (`HUT`) : make account history persistent, also connect it to AEA IDs, also put it in its own class
struct Account {
  int64_t balance = 0;
  std::vector< Transaction > history;
};

// Core OEF implementation
class NodeOEF {
public:
  template <typename T>
  NodeOEF(service::ServiceServer<T> *service, network::ThreadManager *tm,
    const schema::Instance &instance, const schema::Endpoint &nodeEndpoint,
    const schema::Endpoints &endpoints) :
    nodeDirectory_{tm, instance, nodeEndpoint, endpoints} {
    AEADirectory_.register_service_instance(service);
  }

    void Start() {
      nodeDirectory_.Start();
    }

    // HTTP debug, def delete this
    std::string RegisterInstance(std::string agentName, schema::Instance instance) {

      //fetch::logger.PrintTimings();

      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      auto result = serviceDirectory_.RegisterAgent(instance, agentName);

      nodeDirectory_.RegisterAgent(agentName, instance);
      fetch::logger.Info("Registering instance: ", instance.dataModel().name(), " by AEA: ", agentName);
      return std::to_string(result);
    }

    // http so no need to remove callback ref
    void DeregisterInstance(std::string agentName, schema::Instance instance) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      nodeDirectory_.DeregisterAgent(agentName);
    }

    std::vector<std::string> Query(std::string agentName, schema::QueryModel query) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

      if (messageHistorySingle_.add(query)) {
      nodeDirectory_.LogEvent(agentName, query);
        return serviceDirectory_.Query(query);
      }

      return std::vector<std::string>();
    }

    std::vector<std::string> AEAQueryMulti(std::string agentName, schema::QueryModelMulti queryMulti) { // TODO: (`HUT`) : make all const ref.
      std::vector<std::string> result;

      fetch::logger.Info("AEA multi query");

      mutex_.lock();
      if (!nodeDirectory_.shouldForward(queryMulti)) {
        fetch::logger.Info("AEA multi query not suitable for forwarding");
      }


      if (messageHistory_.add(queryMulti) && nodeDirectory_.shouldForward(queryMulti)) {
        fetch::logger.Info("AEA multi query is suitable");
        auto agents = serviceDirectory_.Query(queryMulti.aeaQuery());

        // log each of these results as an event
        for (auto &i : agents) {
          nodeDirectory_.LogEvent(i, queryMulti);
        }

        result.insert(result.end(), agents.begin(), agents.end());

        nodeDirectory_.LogEvent(agentName, queryMulti, true);

        nodeDirectory_.ForwardQuery(queryMulti);
        mutex_.unlock();

        // Wait here for possible query results
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        mutex_.lock();
        agents = nodeDirectory_.ForwardQueryResult(queryMulti);
        result.insert(result.end(), agents.begin(), agents.end());
      }

      mutex_.unlock();
      fetch::logger.Info("AEA multi query is returning");
      return result;
    }

    std::vector<std::pair<schema::Instance, fetch::script::Variant>> QueryAgentsInstances(schema::QueryModel query) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return serviceDirectory_.QueryAgentsInstances(query);
    }

    script::Variant ServiceDirectory() {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return serviceDirectory_.variant();
    }

    std::string test() {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return std::string{"this is a test"};
    }

    schema::Instance getInstance() {
      return nodeDirectory_.getInstance();
    }

    void setInstance(schema::Instance instance) {
      return nodeDirectory_.setInstance(instance);
    }

    // Ledger functionality
    bool IsLedgerUser(const fetch::byte_array::BasicByteArray &user) { // TODO: (`HUT`) : consider whether to distinguish between ledger and AEA users
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return IsLedgerUserPriv(user);
    }

    bool AddLedgerUser(const fetch::byte_array::BasicByteArray &user) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

      if (IsLedgerUserPriv(user)) {
        return false;
      }

      users_.insert(user);
      accounts_[user].balance = 300 + (lfg_() % 9700);

      return true;
    }

    int64_t GetUserBalance(const fetch::byte_array::BasicByteArray &user) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return accounts_[user].balance;
    }

    // TODO: (`HUT`) : make this json doc a set of variants instead
    script::Variant SendTransaction(const fetch::json::JSONDocument &jsonDoc) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

      Transaction tx;
      script::Variant result = script::Variant::Object();

      // TODO: (`HUT`) : some sort of error checking for this
      tx.fromAddress = jsonDoc["fromAddress"].as_byte_array();
      tx.amount      = jsonDoc["balance"].as_int();
      tx.notes       = jsonDoc["notes"].as_byte_array();
      tx.time        = jsonDoc["time"].as_int();
      tx.toAddress   = jsonDoc["toAddress"].as_byte_array();
      tx.json        = jsonDoc.root();

      if ((users_.find(tx.fromAddress) == users_.end())) {
        result["response"] = "fail";
        result["reason"]   = "fromAddress does not exist";
        return result;
      }

      if ((users_.find(tx.toAddress) == users_.end())) {
        result["response"] = "fail";
        result["reason"]   = "toAddress does not exist";
        return result;
      }

      if (accounts_.find(tx.fromAddress) == accounts_.end()) {
        accounts_[tx.fromAddress].balance = 0;
      }

      if (accounts_[tx.fromAddress].balance < tx.amount) {
        result["response"] = "fail";
        result["reason"]   = "Insufficient funds";
        return result;
      }

      accounts_[tx.fromAddress].balance -= tx.amount;
      accounts_[tx.toAddress].balance   += tx.amount;

      accounts_[tx.fromAddress].history.push_back(tx);
      accounts_[tx.toAddress].history.push_back(tx);

      result["response"] = "success";
      result["reason"]   = accounts_[tx.fromAddress].balance;
      return result;
    }

    script::Variant GetHistory(const fetch::byte_array::BasicByteArray &address) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

      auto &account = accounts_[address];
      std::size_t n = std::min(20, static_cast<int>(account.history.size()) );

      script::Variant result = script::Variant::Object();
      script::Variant history = script::Variant::Array(n);

      if (users_.find(address) == users_.end()) {
        result["response"] = "fail";
        result["reason"]   = "toAddress does not exist";
        return result;
      }

      for (std::size_t i=0; i < n; ++i) {
        history[i] = account.history[ account.history.size() - 1 - i].json;
      }

      result["value"] = history;
      result["response"] = "success";

      return result;
    }

    void RegisterCallback(uint64_t client, std::string id, schema::Instance instance) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      AEADirectory_.Register(client, id);
      nodeDirectory_.RegisterAgent(id, instance);
    }

    void DeregisterCallback(uint64_t client, std::string id) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      AEADirectory_.Deregister(client, id);
      nodeDirectory_.DeregisterAgent(id); // TODO: (`HUT`) : I think we want this
    }

    std::string BuyFromAEA(std::string buyer, std::string buyee) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      auto res = AEADirectory_.BuyFromAEA(buyer, buyee);
      std::ostringstream result;
      result << res;
      return result.str();
    }

    script::Variant BuyFromAEA(std::string buyer, const fetch::byte_array::BasicByteArray &id) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return AEADirectory_.BuyFromAEA(buyer, id);
    }

    // Debug functionality
    void AddEndpoint(schema::Endpoint endpoint, schema::Instance instance, schema::Endpoints endpoints) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      fetch::logger.Info("Received add endpoint call");
      nodeDirectory_.AddEndpoint(endpoint, instance, endpoints);
      fetch::logger.Info("Finished add endpoint call");
    }

    // debug adding connection
    void DebugAddConnection(schema::Endpoint endpoint, schema::Endpoint connection) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      nodeDirectory_.DebugAddConnection(endpoint, connection);
    }

    void addConnection(schema::Endpoint endpoint) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      nodeDirectory_.addConnection(endpoint);
    }

    void UpdateEndpoint(schema::Endpoint endpoint, schema::Instance instance) {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      nodeDirectory_.UpdateEndpoint(endpoint, instance);
    }


    void PingAllAEAs() {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      AEADirectory_.PingAllAEAs();
    }

    std::string ping() {
      return "Pinged this Node!";
    }

    // Debug pass results to HTTP interface
    script::Variant DebugAllNodes() {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return nodeDirectory_.DebugAllNodes();
    }

    script::Variant DebugAllEndpoints() {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return nodeDirectory_.DebugAllEndpoints();
    }

    script::Variant DebugConnections() {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return nodeDirectory_.DebugConnections();
    }

    script::Variant DebugEndpoint() {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      return nodeDirectory_.DebugEndpoint();
    }

    void GetAgents() {
      std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
      AEADirectory_.PingAllAEAs();
    }

    void ForwardQuery(std::string name, schema::Endpoint endpoint, schema::QueryModelMulti queryMulti) {

      std::vector<std::string> result;
      mutex_.lock();
      if (messageHistory_.add(queryMulti) && nodeDirectory_.shouldForward(queryMulti)) {

        nodeDirectory_.LogEvent(name, queryMulti);

        auto agents = serviceDirectory_.Query(queryMulti.aeaQuery());

        for (auto &i : agents) {
          nodeDirectory_.LogEventReverse(i, queryMulti);   // this will log Node -> AEA
        }

        mutex_.unlock();

        nodeDirectory_.ForwardQuery(endpoint, queryMulti);
        nodeDirectory_.ReturnQuery(queryMulti, agents);
      } else {
        fetch::logger.Info("Not forwarding query: ", name);
        fetch::logger.Info("Match to our instance: ", nodeDirectory_.shouldForward(queryMulti));
      }

      mutex_.unlock();
    }

    void ReturnQuery(schema::QueryModelMulti queryMulti, std::vector<std::string> agents) {
        nodeDirectory_.ReturnQuery(queryMulti, agents);
    }

    // Pass through functions for node dir (have their own mutexes) debugging TODO: (`HUT`) : make varargs
    void addAgent(schema::Endpoint endpoint, std::string agent, schema::Instance instance) { nodeDirectory_.addAgent(endpoint, agent, instance); }
    void removeAgent(schema::Endpoint endpoint, std::string agent) { nodeDirectory_.removeAgent(endpoint, agent); }

    void logEvent(schema::Endpoint endpoint, Event event) { nodeDirectory_.logEvent(endpoint, event); }

    // HTTP returns
    script::Variant DebugAllAgents()              { return nodeDirectory_.DebugAllAgents(); }
    script::Variant DebugAllEvents(int maxNumber) { return nodeDirectory_.DebugAllEvents(maxNumber); }

    // TODO: (`HUT`) : consider whether this should be private, protected or public (public for debug version)
    NodeDirectory         nodeDirectory_;

  private:
    oef::ServiceDirectory                   serviceDirectory_;
    AEADirectory                            AEADirectory_;
    MessageHistory<schema::QueryModelMulti> messageHistory_;
    MessageHistory<schema::QueryModel>      messageHistorySingle_;

    // Ledger
    std::vector< Transaction >                             transactions_;
    std::map< fetch::byte_array::BasicByteArray, Account > accounts_;
    std::set< fetch::byte_array::BasicByteArray >          users_;
    fetch::random::LaggedFibonacciGenerator<>              lfg_;
    fetch::mutex::Mutex                                    mutex_;

    bool IsLedgerUserPriv(const fetch::byte_array::BasicByteArray &user) const {
      return (users_.find(user) != users_.end());
    }
};
}
}

#endif
