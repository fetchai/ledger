// Company: FETCH.ai
// Author: Jerome Maloberti
// Creation: 09/01/18
//
// This file holds the implementation for a serviceDirectory. It's responsibilities are to manage a list of agents, and their services

#pragma once
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <mutex>

// Fetch.ai
//#include "serialize.h"
//#include "proxy.h"
#include "oef/schema.h"

class Agents {

 public:
  explicit Agents() {}
  explicit Agents(const std::string &agent) {insert(agent);}

  bool insert(const std::string &agent)
  {
    return _agents.insert(agent).second;
  }

  bool erase(const std::string &agent)
  {
    return _agents.erase(agent) == 1;
  }

  bool contains(const std::string &agent)
  {
    return _agents.find(agent) != _agents.end();
  }

  size_t size() const
  {
    return _agents.size();
  }

  void copy(std::unordered_set<std::string> &s) const
  {
    std::copy(_agents.begin(), _agents.end(), std::inserter(s, s.end()));
  }

 private:
  std::unordered_set<std::string> _agents;
};

class ServiceDirectory {

 public:
  explicit ServiceDirectory() = default;

  bool registerAgent(const Instance &instance, const std::string &agent)
  {
    std::lock_guard<std::mutex> lock(_lock);
    return _data[instance].insert(agent);
  }

  bool unregisterAgent(const Instance &instance, const std::string &agent)
  {
    std::lock_guard<std::mutex> lock(_lock);
    auto iter = _data.find(instance);
    if(iter == _data.end())
      return false;
    bool res = iter->second.erase(agent);
    if(iter->second.size() == 0) {
      _data.erase(instance);
    }
    return res;
  }

  bool remove(const std::string &agent)
  {
    std::lock_guard<std::mutex> lock(_lock);
    bool res = false;

    for (auto iter = _data.begin(); iter != _data.end(); ++iter )
    {
      if (iter->second.contains(agent))
      {
        res = iter->second.erase(agent);

        if(iter->second.size() == 0)
        {
          _data.erase(iter->first);
        }
        break; // NOTE: We assume here that agents will only be registered with one service.
      }
    }

    return res;
  }

  size_t size() const
  {
    std::lock_guard<std::mutex> lock(_lock);
    return _data.size();
  }

  std::vector<std::string> query(const QueryModel &query) const
  {
    std::lock_guard<std::mutex> lock(_lock);
    std::unordered_set<std::string> res;
    for(auto &d : _data)
    {
      if(query.check(d.first))
      {
        d.second.copy(res);
      }
    }
    return std::vector<std::string>(res.begin(), res.end());
  }

 private:
  mutable std::mutex _lock;
  std::unordered_map<Instance, Agents> _data;
};
