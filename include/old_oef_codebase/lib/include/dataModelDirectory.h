// Company: FETCH.ai
// Author: Nathan Hutton
// Creation: 22/02/18
//
// The DataModel directory holds all of the DataModels that have been registered, and the agents associated with them. It is possible to
// search for DataModels by keyword, from that it should be possible to search for Agents using a normal query. Agents are paired to DataModels
// to use when cleaning up

#pragma once

// Fetch.ai
#include "schema.h"
#include "serviceDirectory.h"

class DataModelDirectory {

 public:
  explicit DataModelDirectory() = default;

  // TODO: (`HUT`) : implement fuller serialize and archive functions
  template <typename Archive>
  void serialize(Archive &ar) const
  {
    std::lock_guard<std::mutex> lock(_lock);
    ObjectWrapper<Archive> o{ar};
    ar.write("DataModelDirectory: ", _data, "DataModel");
  }

  // Note, will return false if it already exists
  // TODO: (`HUT`) : create slightly more efficient lookup
  bool registerDataModel(const DataModel &dataModel, const std::string &agent)
  {
    std::cout << "this happenend\n\n\n";
    std::lock_guard<std::mutex> lock(_lock);

    // Look through all pairs for the DataModel
    for (auto iter = _data.begin(); iter != _data.end(); ++iter )
    {
      if(iter->first == dataModel)
      {
        std::cerr << "dm already exists!" << std::endl;
        return iter->second.insert(agent);
      }
    }

    // DM doesn't exist already
    _data.push_back(std::pair<DataModel, Agents> {dataModel, Agents{agent}});
    return true;
  }

  void keywordLookup(const KeywordLookup &lookup)
  {
    std::cerr << "we are at the lookup" << std::endl;
    std::cerr << "thing thing" << toJsonString<KeywordLookup>(lookup) << std::endl;
    for(auto i : lookup.getKeywords())
    {
      std::cerr << i << std::endl;
    }

  }

  bool remove(const std::string &agent)
  {
    std::lock_guard<std::mutex> lock(_lock);
  }

  size_t size() const
  {
    std::lock_guard<std::mutex> lock(_lock);
    return _data.size();
  }

 private:
  mutable std::mutex _lock;
  std::vector<std::pair<DataModel, Agents>> _data;
};

