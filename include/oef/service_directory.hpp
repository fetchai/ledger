// This file holds the implementation for a serviceDirectory. It's responsibilities are to manage a list of agents, and their services (instance of datamodel).
// It also responds to 'queries', returning the agents that meet these queries

#ifndef SERVICE_DIRECTORY_HPP
#define SERVICE_DIRECTORY_HPP

#include<unordered_map>
#include<unordered_set>
#include"oef/schema.hpp"

namespace fetch
{
namespace oef
{

// ServiceDirectory holds a list of *instances* and the agents associated with those instances. The assumption is therefore that the number of instances is equal to or less than agents
class ServiceDirectory {
public:
  explicit ServiceDirectory() = default;

  bool RegisterAgent(const schema::Instance &instance, const std::string &agent) {
    return data_[instance].Insert(agent);
  }

  bool UnregisterAgent(const schema::Instance &instance, const std::string &agent) {
    auto iter = data_.find(instance);
    if(iter == data_.end())
      return false;
    bool res = iter->second.Erase(agent);
    if(iter->second.size() == 0) {
      data_.erase(instance);
    }
    return res;
  }

  bool Remove(const std::string &agent) {
    bool res = false;

    for (auto iter = data_.begin(); iter != data_.end(); ++iter ) {
      if (iter->second.Contains(agent)) {
        res = iter->second.Erase(agent);

        if(iter->second.size() == 0) {
          data_.erase(iter->first);
        }
        break; // NOTE: We assume here that agents will only be registered with one service.
      }
    }
    return res;
  }

  std::vector<std::string> Query(const schema::QueryModel &query) const {
    std::unordered_set<std::string> res;

    for(auto &d : data_) {
      if(query.check(d.first)) {
        d.second.Copy(res);
      }
    }
    return std::vector<std::string>(res.begin(), res.end());
  }

  size_t size() const {
    return data_.size();
  }

 std::vector<std::pair<schema::Instance, fetch::script::Variant>> QueryAgentsInstances(const schema::QueryModel &query) const {

    std::vector<std::pair<schema::Instance, fetch::script::Variant>> res;

    for(auto &d : data_) {

      std::ostringstream find; // TODO: (`HUT`) : remove this
      find << d.first.variant();
      std::cerr << "THEIR instance is " << find.str() << std::endl;
      std::cerr << "OUR constraint is " << query.variant() << std::endl;

      if(query.check(d.first)) {
        std::pair<schema::Instance, fetch::script::Variant> pushThis(d.first, d.second.variant());
        res.push_back(pushThis);
      }
    }

    return res;
  }

 script::Variant variant() {
    fetch::script::Variant res = fetch::script::Variant::Array(data_.size());

    int index = 0;
    for(auto &i : data_){

      fetch::script::Variant temp = fetch::script::Variant::Object();
      temp["service"] = i.first.variant();
      temp["agents"]  = i.second.variant();
      res[index++] = temp;
    }

    return res;
 }

private:
  std::unordered_map<schema::Instance, schema::Agents> data_;
};

}
}
#endif
