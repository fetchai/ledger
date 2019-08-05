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

#include "storage/object_store.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace fetch {
namespace storage {

/**
 * Stores objects of type T in a stack type structure by using an object store,
 * where push and pop use uint64_t to string as the key
 */
template <typename T, std::size_t S = 2048>
class ObjectStack : public ObjectStore<T, S>
{
public:
  void Push(T const &object)
  {
    return this->Set(storage::ResourceAddress(std::to_string(this->size())), object);
  }

  void Pop()
  {
    assert(this->size() > 0);
    return this->Erase(storage::ResourceAddress(std::to_string(this->size() - 1)));
  }

  /**
   * Overloads the Get from the parent to take an index instead
   *
   * @param: index The index to get
   * @param: object The object to return
   *
   * @return: bool Whether the operation was a success (safe to try and get elements bigger than
   * stack size)
   */
  bool Get(uint64_t index, T &object)
  {
    return ObjectStore<T, S>::Get(storage::ResourceAddress(std::to_string(index)).as_resource_id(),
                                  object);
  }
};

}  // namespace storage
}  // namespace fetch
