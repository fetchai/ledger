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

#include "variant/variant.hpp"

namespace fetch {
namespace variant {

/**
 * Variant assignment operator
 *
 * @param value The value to be assigned
 * @return Reference to the current object
 */
Variant& Variant::operator=(Variant const &value)
{
  Reset();

  // update the type
  type_ = value.type_;

  // based on the new type update accordingly
  switch (type_)
  {
    case Type::UNDEFINED:
    case Type::NULL_VALUE:
      // empty types are conveyed based on the type field alone
      break;

    case Type::BOOLEAN:
    case Type::INTEGER:
    case Type::FLOATING_POINT:
      primitive_ = value.primitive_;
      break;

    case Type::STRING:
      string_ = value.string_;
      break;

    case Type::ARRAY:
      ResizeArray(value.array_.size());
      for (std::size_t i = 0, end = value.array_.size(); i < end; ++i)
      {
        *(array_.at(i)) = *(value.array_.at(i));
      }
      break;

    case Type::OBJECT:
    {
      for (auto const &element : value.object_)
      {
        // create an element
        Variant *obj = pool().Allocate();
        obj->parent_ = parent();

        // make a deep copy
        *obj = *element.second;

        // update our object
        object_.emplace(element.first, obj);
      }
      break;
    }
  }

  return *this;
}

/**
 * Internal: Reset the variant to its undefined state, releasing all held resources
 */
void Variant::Reset()
{
  switch (type_)
  {
    case Type::UNDEFINED:
    case Type::NULL_VALUE:
    case Type::INTEGER:
    case Type::FLOATING_POINT:
    case Type::BOOLEAN:
      break;
    case Type::STRING:
      string_ = ConstByteArray();
      break;

    case Type::ARRAY:
      ResizeArray(0);
      break;

    case Type::OBJECT:
    {
      Variant *parent = (parent_) ? parent_ : this;

      auto it = object_.begin();
      while (it != object_.end())
      {
        Variant *variant = it->second;
        it = object_.erase(it);

        assert(variant->parent_ == parent);
        variant->Reset();
        variant->parent_ = nullptr;

        pool().Release(variant);

      }
      break;
    }
  }

  type_ = Type::UNDEFINED;
}

} // namespace variant
} // namespace fetch
