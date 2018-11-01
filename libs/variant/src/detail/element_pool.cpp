//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "variant/detail/element_pool.hpp"

namespace fetch {
namespace variant {
namespace detail {

bool Element::operator==(Element const &other) const
{
  bool equal{false};

  if (type == other.type)
  {
    switch (type)
    {
      case Type::UNDEFINED:
      case Type::NULL_VALUE:
        equal = true;
        break;

      case Type::INTEGER:
        equal = primitive.integer == other.primitive.integer;
        break;

      case Type::FLOATING_POINT:
        equal = primitive.float_point == other.primitive.float_point;
        break;

      case Type::BOOLEAN:
        equal = primitive.boolean == other.primitive.boolean;
        break;

      case Type::STRING:
        equal = string == other.string;
        break;

      case Type::ARRAY:
      {
        equal = array.size() == other.array.size();

        if (equal)
        {
          // check the contents
          for (std::size_t i = 0; i < array.size(); ++i)
          {
            // if the pointers are different and the contents are different
            if ((array[i] != other.array[i]) && (*array[i] != *other.array[i]))
            {
              equal = false;
              break;
            }
          }
        }
        break;
      }

      case Type::OBJECT:
      {
        equal = object.size() == other.object.size();

        for (auto const &element : object)
        {
          // lookup key in the other array
          auto it = other.object.find(element.first);
          if (it == other.object.end())
          {
            equal = false;
            break;
          }

          // if the pointers are different and the contents are different
          if ((element.second != it->second) && (*element.second != *(it->second)))
          {
            equal = false;
            break;
          }
        }

        break;
      }
    }
  }

  return equal;
}

} // namespace detail
} // namespace variant
} // namespace fetch
