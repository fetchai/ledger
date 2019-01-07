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

#include <typeinfo>
#include <vector>

namespace fetch {
namespace vm {

namespace details {

template <typename... Args>
struct ArgumentsToList;

template <typename T, typename... Args>
struct ArgumentsToList<T, Args...>
{

  static void AppendTo(std::vector<std::type_index> &list)
  {
    list.push_back(std::type_index(typeid(T)));
    ArgumentsToList<Args...>::AppendTo(list);
  }
};

template <typename T>
struct ArgumentsToList<T>
{

  static void AppendTo(std::vector<std::type_index> &list)
  {
    list.push_back(std::type_index(typeid(T)));
  }
};

template <>
struct ArgumentsToList<>
{
  static void AppendTo(std::vector<std::type_index> &list)
  {}
};

}  // namespace details
}  // namespace vm
}  // namespace fetch
