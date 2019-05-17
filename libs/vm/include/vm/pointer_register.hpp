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

#include <unordered_map>

namespace fetch {
namespace vm {

class PointerRegister
{
public:
  template <typename T>
  void Set(T *val)
  {
    pointers_[std::type_index(typeid(T))] = static_cast<void *>(val);
  }

  template <typename T>
  T *Get()
  {
    if (pointers_.find(std::type_index(typeid(T))) == pointers_.end())
    {
      return nullptr;
    }

    return static_cast<T *>(pointers_[std::type_index(typeid(T))]);
  }

private:
  std::unordered_map<std::type_index, void *> pointers_;
};

}  // namespace vm
}  // namespace fetch