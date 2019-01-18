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

#include "variant/variant.hpp"

namespace fetch {
namespace variant {

/**
 * Attempts to extract a value from a variant object
 *
 * @tparam T The type of the value to extract
 * @param object The object to extract from
 * @param key The key to lookup
 * @param value The destination for the extracted value
 * @return true if successful, otherwise false
 */
template <typename T>
bool Extract(Variant const &object, byte_array::ConstByteArray const &key, T &value)
{
  bool success{false};

  // ensure the input variant is an object and has be desired key
  if (object.IsObject() && object.Has(key))
  {
    Variant const &element = object[key];

    // ensure that the element is compatible with the requested byte
    if (element.Is<T>())
    {
      // extract / convert the element to the requested type
      value = element.As<T>();

      success = true;
    }
  }

  return success;
}

inline bool Extract(Variant const &object, byte_array::ConstByteArray const &key, Variant &value)
{
  if (!object.Has(key))
  {
    return false;
  }

  value = object[key];
  return value.IsObject();
}

}  // namespace variant
}  // namespace fetch