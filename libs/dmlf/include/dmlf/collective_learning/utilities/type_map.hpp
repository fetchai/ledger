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

#include <atomic>
#include <exception>
#include <iostream>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace fetch {
namespace dmlf {

template <typename ValueType = std::string>
class TypeMap
{
public:
  using IntToValueMap = std::unordered_map<std::type_index, ValueType>;

  template <typename KeyType>
  ValueType find() const
  {
    std::type_index tid{typeid(KeyType)};
    auto            iter = map_.find(tid);
    if (iter == map_.end())
    {
      throw std::runtime_error{"Type not registered"};
    }
    return iter->second;
  }

  template <typename KeyType>
  void put(ValueType value)
  {
    std::type_index tid{typeid(KeyType)};
    auto            iter = map_.find(tid);
    if (iter != map_.end())
    {
      throw std::runtime_error{"Type already registered with name '" + iter->second + "'"};
    }
    map_[tid] = value;
  }

private:
  IntToValueMap map_;
};

}  // namespace dmlf
}  // namespace fetch
