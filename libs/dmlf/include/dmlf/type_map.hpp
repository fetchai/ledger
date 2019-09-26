#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <iostream>
#include <atomic>
#include <unordered_map>
#include <exception>

namespace fetch {
namespace dmlf  {

template <typename ValueType = std::string>
class TypeMap {
public:
  using IntToValueMap = std::unordered_map<int, ValueType>;
  
  template <typename KeyType>
  ValueType find() const
  {
    auto iter = map.find(getTypeId<KeyType>());
    if (iter == map.end())
    {
      throw std::runtime_error{"Type not registered"};
    }
    return iter->second;
  }

  template<typename KeyType>
  //void put(ValueType &&value)
  void put(ValueType value)
  {
    auto iter = map.find(getTypeId<KeyType>());
    if (iter != map.end())
    {
      throw std::runtime_error{"Type already registered with name '"+iter->second+"'"};
    }
    //map[getTypeId<KeyType>()] = std::forward<ValueType>(value);
    map[getTypeId<KeyType>()] = value;
  }

  template <typename KeyType>
  inline static int getTypeId()
  {
    static const int id = LastTypeId++;
    return id;
  }

private:
  static std::atomic_int LastTypeId;
  IntToValueMap map;
};

template<typename ValueType>
std::atomic_int TypeMap<ValueType>::LastTypeId(0);

}  // namespace dmlf
}  // namespace fetch

