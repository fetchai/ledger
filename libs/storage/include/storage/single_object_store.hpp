#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/serializers/main_serializer.hpp"
#include <fstream>

namespace fetch {
namespace storage {

/**
 * Stores and loads a single object, providing serialization functionality
 * also. The user must take care to Get and Set the same types - note this
 * class is likely to throw if there is an error and so should be wrapped
 */
class SingleObjectStore
{
public:
  using ByteArray = byte_array::ByteArray;

  SingleObjectStore()            = default;
 ~SingleObjectStore();
  SingleObjectStore(SingleObjectStore const &rhs)            = delete;
  SingleObjectStore(SingleObjectStore &&rhs)                 = delete;
  SingleObjectStore &operator=(SingleObjectStore const &rhs) = delete;
  SingleObjectStore &operator=(SingleObjectStore&& rhs)      = delete;

  /**
   * Load a file, creating it if it does not exist. Will throw
   * if the file is not the correct version and format, or if
   * loading fails due to corruption
   */
  void Load(std::string const &doc_file);

  /**
   * Get the version of the file that has been loaded
   */
  uint16_t Version() const;

  /**
   * Get a byte array to the file data
   */
  void GetRaw(ByteArray &data) const;

  /**
   * Set the file data
   */
  void SetRaw(ByteArray &data);

  /**
   * Get an object from the file
   */
  template <typename T>
  void Get(T &object) const
  {
    ByteArray data;

    GetRaw(data);

    serializers::LargeObjectSerializeHelper serializer{data};

    serializer >> object;
  }

  /**
   * Set an object to the file
   */
  template <typename T>
  void Set(T const &object)
  {
    serializers::LargeObjectSerializeHelper serializer{};
    serializer << object;

    ByteArray array{serializer.data()};

    SetRaw(array);
  }

  /**
   * Close the file
   */
  void Close();

private:
  uint16_t version_{1};
  mutable std::fstream file_handle_;
};

}  // namespace storage
}  // namespace fetch
