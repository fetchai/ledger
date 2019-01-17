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

#include "core/serializers/stl_types.hpp"
#include <vector>
namespace fetch {
namespace serializers {

template <typename T>
void Serialize(T &serializer, std::vector<std::string> const &vec)
{
  // Allocating memory for the size
  serializer.Allocate(sizeof(uint64_t));
  uint64_t size = vec.size();

  // Writing the size to the byte array
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size), sizeof(uint64_t));

  for (auto const &a : vec)
  {
    serializer << a;
  }
}

template <typename T>
void Deserialize(T &serializer, std::vector<std::string> &vec)
{
  uint64_t size;
  // Writing the size to the byte array
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));

  // Reading the data
  vec.resize(size);

  for (auto &a : vec)
  {
    serializer >> a;
  }
}

}  // namespace serializers
}  // namespace fetch
