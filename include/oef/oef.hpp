#ifndef OEF_HPP
#define OEF_HPP

#include<iostream>
#include<set>
#include<map>
#include<string>
#include"service/server.hpp"
#include"oef/schema.hpp"
#include"oef/schema_serializers.hpp"
#include"oef/service_directory.hpp"
#include"protocols/fetch_protocols.hpp"
#include"protocols/node_to_aea/commands.hpp"

namespace fetch
{
namespace oef
{

class ListeningAEAs {
public:
  void Register(uint64_t client, std::string id) {
    std::cout << "\rRegistering " << client << " with id " << id << std::endl << std::endl << "> " << std::flush;

    mutex_.lock();
    registered_aeas_[client] = id; // TODO: (`HUT`) : proper management of this
    mutex_.unlock();
  }

  void Deregister(uint64_t client, std::string id) {
    std::cout << "\rDeregistering " << client << " with id " << id << std::endl << std::endl << "> " << std::flush;
    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

    registered_aeas_[client] = ""; // TODO: (`HUT`) : proper management of this (check corresponding id)
  }

  void PingAllAEAs() {
    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

    detailed_assert( service_ != nullptr);

    for(auto &id: registered_aeas_) {
      auto &rpc = service_->ServiceInterfaceOf(id.first);

      rpc.Call(protocols::FetchProtocols::NODE_TO_AEA, protocols::NodeToAEAReverseRPC::PING, "ping_message"); // TODO: (`HUT`) : think about decoupling
    }
  }

  script::Variant BuyFromAEA(const fetch::byte_array::BasicByteArray &id) {
    script::Variant result = script::Variant::Object();
    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

    std::string aeaID{id};

    for (std::map<uint32_t,std::string>::const_iterator it=registered_aeas_.begin(); it!=registered_aeas_.end(); ++it){
      if(aeaID.compare(it->second) == 0){
        result["response"] = "success";

        auto &rpc = service_->ServiceInterfaceOf(it->first);
        std::string answer   = rpc.Call(protocols::FetchProtocols::NODE_TO_AEA, protocols::NodeToAEAReverseRPC::BUY, "http_interface").As<std::string>();
        result["value"]   = answer;
        return result;
      }
    }

    result["response"] = "fail"; // TODO: (`HUT`) : ask troels about building variants like var = "thing " = vari + "more";
    std::string build{"AEA id: '"};
    build += id;
    build += "' not active";
    result["reason"]   = build;

    return result;
  }

  void register_service_instance( service::ServiceServer< fetch::network::TCPServer > *ptr)
  {
    service_ = ptr;
  }

private:
  service::ServiceServer< fetch::network::TCPServer > *service_ = nullptr;
  std::map< uint32_t, std::string >                    registered_aeas_;
  fetch::mutex::Mutex                                  mutex_;
};

struct Transaction
{
  int64_t               amount;
  byte_array::ByteArray fromAddress;
  byte_array::ByteArray notes;
  uint64_t              time;
  byte_array::ByteArray toAddress;
  script::Variant       json;
};

// TODO: (`HUT`) : make account history persistent
struct Account
{
  int64_t balance = 0;
  std::vector< Transaction > history;
};

// Core OEF implementation
class NodeOEF {

public:

  template <typename T>
  NodeOEF(service::ServiceServer<T> *service) { register_service_instance(service); }

  std::string RegisterInstance(std::string agentName, schema::Instance instance) {
    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
    auto result = serviceDirectory_.RegisterAgent(instance, agentName);

    fetch::logger.Info("Registering instance: ", instance.dataModel().name(), " by AEA: ", agentName);
    return std::to_string(result);
  }

  std::vector<std::string> Query(schema::QueryModel query) {
    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
    return serviceDirectory_.Query(query);
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

  // Ledger functionality
  bool IsLedgerUser(const fetch::byte_array::BasicByteArray &user) { // TODO: (`HUT`) : consider whether to distinguish between ledger and AEA users
    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);
    return IsLedgerUserPriv(user);
  }

  bool AddLedgerUser(const fetch::byte_array::BasicByteArray &user) {
    std::lock_guard< fetch::mutex::Mutex > lock(mutex_);

    if(IsLedgerUserPriv(user)) {
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

    if((users_.find(tx.fromAddress) == users_.end())){
      result["response"] = "fail";
      result["reason"]   = "fromAddress does not exist";
      return result;
    }

    if((users_.find(tx.toAddress) == users_.end())) {
      result["response"] = "fail";
      result["reason"]   = "toAddress does not exist";
      return result;
    }

    if(accounts_.find(tx.fromAddress) == accounts_.end()) {
      accounts_[tx.fromAddress].balance = 0;
    }

    if(accounts_[tx.fromAddress].balance < tx.amount) {
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

    auto &account = accounts_[address];
    std::size_t n = std::min(20, int(account.history.size()) );

    script::Variant result = script::Variant::Object();
    script::Variant history = script::Variant::Array(n);

    if(users_.find( address ) == users_.end()) {
      result["response"] = "fail";
      result["reason"]   = "toAddress does not exist";
      return result;
    }

    for(std::size_t i=0; i < n; ++i)
    {
      history[i] = account.history[ account.history.size() - 1 - i].json;
    }

    result["value"] = history;
    result["response"] = "success";

    return result;
 }


 void RegisterCallback(uint64_t client, std::string id) {
   listeningAEAs_.Register(client, id);
 }

 void DeregisterCallback(uint64_t client, std::string id) {
   listeningAEAs_.Deregister(client, id);
 }

 void PingAllAEAs() {
   listeningAEAs_.PingAllAEAs();
 }

 std::string BuyFromAEA(std::string id) {
    auto res = listeningAEAs_.BuyFromAEA(id);
    std::ostringstream result;
    result << res;
    return result.str();
 }

 script::Variant BuyFromAEA(const fetch::byte_array::BasicByteArray &id) {
    return listeningAEAs_.BuyFromAEA(id);
 }

  void register_service_instance( service::ServiceServer< fetch::network::TCPServer > *ptr) {
    listeningAEAs_.register_service_instance(ptr);
  }

private:
  service_directory::ServiceDirectory serviceDirectory_;
  ListeningAEAs                       listeningAEAs_;

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
