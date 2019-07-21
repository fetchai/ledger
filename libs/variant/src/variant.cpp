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

#include "core/macros.hpp"
#include "variant/variant.hpp"

#include <cstddef>
#include <iomanip>
#include <ostream>

namespace fetch {
namespace variant {

/**
 * Variant assignment operator
 *
 * @param value The value to be assigned
 * @return Reference to the current object
 */
Variant &Variant::operator=(Variant const &value)
{
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
  case Type::FIXED_POINT:
    primitive_ = value.primitive_;
    break;

  case Type::STRING:
    string_ = value.string_;
    break;

  case Type::ARRAY:
    ResizeArray(value.array_.size());
    for (std::size_t i = 0, end = value.array_.size(); i < end; ++i)
    {
      array_.at(i) = value.array_.at(i);
    }
    break;

  case Type::OBJECT:
  {
    for (auto const &element : value.object_)
    {
      std::unique_ptr<Variant> variant{new Variant(*element.second)};
      // update our object
      object_.emplace(element.first, std::move(variant));
    }
    break;
  }
  }

  return *this;
}

/**
 * Check for equality between to variant objects
 *
 * @param other The other variant to compare against
 * @return true if the two variant are equal, otherwise false
 */
bool Variant::operator==(Variant const &other) const
{
  bool equal{false};

  if (type_ == other.type_)
  {
    switch (type_)
    {
    case Type::UNDEFINED:
    case Type::NULL_VALUE:
      equal = true;
      break;

    case Type::INTEGER:
      equal = primitive_.integer == other.primitive_.integer;
      break;

    case Type::FLOATING_POINT:
      equal = primitive_.float_point == other.primitive_.float_point;
      break;

    case Type::FIXED_POINT:
      equal = primitive_.integer == other.primitive_.integer;
      break;

    case Type::BOOLEAN:
      equal = primitive_.boolean == other.primitive_.boolean;
      break;

    case Type::STRING:
      equal = string_ == other.string_;
      break;

    case Type::ARRAY:
    {
      equal = array_.size() == other.array_.size();

      if (equal)
      {
        // check the contents
        for (std::size_t i = 0; i < array_.size(); ++i)
        {
          // if the pointers are different and the contents are different
          if (array_[i] != other.array_[i])
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
      equal = object_.size() == other.object_.size();

      for (auto const &element : object_)
      {
        // lookup key in the other array
        auto it = other.object_.find(element.first);
        if (it == other.object_.end())
        {
          equal = false;
          break;
        }

        // if the pointers are different and the contents are different
        if (*(element.second) != *(it->second))
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

/**
 * Stream variant object out as a JSON encoded sequence
 *
 * @param stream The output stream
 * @param variant The input variant
 * @return The output stream
 */
std::ostream &operator<<(std::ostream &stream, Variant const &variant)
{
  using fetch::byte_array::ConstByteArray;

  switch (variant.type_)
  {
  case Variant::Type::UNDEFINED:
    stream << "(undefined)";
    break;

  case Variant::Type::INTEGER:
    stream << variant.As<int64_t>();
    break;

  case Variant::Type::FLOATING_POINT:
    stream << variant.As<double>();
    break;

  case Variant::Type::FIXED_POINT:
    stream << variant.As<fixed_point::fp64_t>();
    break;

  case Variant::Type::STRING:
    stream << std::quoted(variant.As<std::string>());
    break;

  case Variant::Type::BOOLEAN:
    stream << (variant.As<bool>() ? "true" : "false");
    break;

  case Variant::Type::NULL_VALUE:
    stream << "null";
    break;

  case Variant::Type::ARRAY:
    stream << "[";

    for (std::size_t i = 0; i < variant.array_.size(); ++i)
    {
      if (i != 0)
      {
        stream << ", ";
      }

      // recursively call the stream operator
      stream << variant.array_[i];
    }

    stream << "]";
    break;

  case Variant::Type::OBJECT:
    stream << "{";

    std::size_t i{0};
    for (auto const &element : variant.object_)
    {
      if (i != 0)
      {
        stream << ", ";
      }

      // format the element
      stream << std::quoted(std::string{element.first}) << ": " << *element.second;

      ++i;
    }

    stream << "}";

    break;
  }

  return stream;
}

}  // namespace variant
}  // namespace fetch
