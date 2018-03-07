// This file holds the implementation for a serviceDirectory. It's responsibilities are to manage a list of agents, and their services (instance of datamodel).
// It also responds to 'queries', returning the agents that meet these queries

#ifndef SERVICE_DIRECTORY_HPP
#define SERVICE_DIRECTORY_HPP

#include<unordered_map>
#include<unordered_set>
#include<mutex>
#include"oef/schema.hpp"

namespace fetch
{
namespace service_directory
{

// The Agents class is just a convenience for representing agents, this will be extended to hold agent-specific information later
class Agents {
public:
  explicit Agents() {}
  explicit Agents(const std::string &agent) {Insert(agent);}

  bool Insert(const std::string &agent) {
    return agents_.insert(agent).second;
  }

  bool Erase(const std::string &agent) {
    return agents_.erase(agent) == 1;
  }

  bool Contains(const std::string &agent) {
    return agents_.find(agent) != agents_.end();
  }

  size_t size() const {
    return agents_.size();
  }

  void Copy(std::unordered_set<std::string> &s) const {
    std::copy(agents_.begin(), agents_.end(), std::inserter(s, s.end()));
  }

private:
  std::unordered_set<std::string> agents_;
};

// ServiceDirectory holds a list of *instances* and the agents associated with those instances. The assumption is therefore that the number of instances is equal to or less than agents
class ServiceDirectory {
public:
  explicit ServiceDirectory() = default;

  bool RegisterAgent(const schema::Instance &instance, const std::string &agent) {
    std::lock_guard<std::mutex> lock(_lock);
    return _data[instance].Insert(agent);
  }

  bool UnregisterAgent(const schema::Instance &instance, const std::string &agent) {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _data.find(instance);
    if(iter == _data.end())
      return false;
    bool res = iter->second.Erase(agent);
    if(iter->second.size() == 0) {
      _data.erase(instance);
    }
    return res;
  }

  bool Remove(const std::string &agent) {
    std::lock_guard<std::mutex> lock(_lock);
    bool res = false;

    for (auto iter = _data.begin(); iter != _data.end(); ++iter ) {
      if (iter->second.Contains(agent)) {
        res = iter->second.Erase(agent);

        if(iter->second.size() == 0) {
          _data.erase(iter->first);
        }
        break; // NOTE: We assume here that agents will only be registered with one service.
      }
    }
    return res;
  }

  std::vector<std::string> Query(const schema::QueryModel &query) const {
    std::lock_guard<std::mutex> lock(_lock);
    std::unordered_set<std::string> res;

    for(auto &d : _data) {
      if(query.check(d.first)) {
        d.second.Copy(res);
      }
    }
    return std::vector<std::string>(res.begin(), res.end());
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock(_lock);
    return _data.size();
  }

private:
  mutable std::mutex                           _lock;
  std::unordered_map<schema::Instance, Agents> _data;
};

}
}
#endif
