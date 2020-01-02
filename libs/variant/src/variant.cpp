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
      auto variant = std::make_unique<Variant>(*element.second);
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
        // look up key in the other array
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

bool Variant::IsUndefined() const
{
  return type_ == Type::UNDEFINED;
}

bool Variant::IsInteger() const
{
  return type_ == Type::INTEGER;
}

bool Variant::IsFloatingPoint() const
{
  return type_ == Type::FLOATING_POINT;
}

bool Variant::IsFixedPoint() const
{
  return type_ == Type::FIXED_POINT;
}

bool Variant::IsBoolean() const
{
  return type_ == Type::BOOLEAN;
}

bool Variant::IsString() const
{
  return type_ == Type::STRING;
}

bool Variant::IsNull() const
{
  return type_ == Type::NULL_VALUE;
}

bool Variant::IsArray() const
{
  return type_ == Type::ARRAY;
}

bool Variant::IsObject() const
{
  return type_ == Type::OBJECT;
}

/**
 * Creates and returns a Null variant
 *
 * @return A null variant
 */
Variant Variant::Null()
{
  Variant v;
  v.type_ = Type::NULL_VALUE;
  return v;
}

/**
 * Creates and returns a undefined variant
 *
 * @return An undefined variant
 */
Variant Variant::Undefined()
{
  return {};
}

/**
 * Creates and returns an array of specified size containing undefined elements
 *
 * @param elements The size of the array variant
 * @return The generated array
 */
Variant Variant::Array(std::size_t elements)
{
  Variant v;
  v.type_ = Type::ARRAY;
  v.ResizeArray(elements);

  return v;
}

/**
 * Creates and returns an empty object variant
 *
 * @return The generated object
 */
Variant Variant::Object()
{
  Variant v;
  v.type_ = Type::OBJECT;

  return v;
}

/**
 * (Deep) copy construct a variant from another variant
 *
 * @param other The other variant to copy from
 */
Variant::Variant(Variant const &other)
  : Variant()
{
  *this = other;
}

/**
 * Raw String constructor
 *
 * @param value The raw string value to be set
 */
Variant::Variant(char const *value)
  : Variant()
{
  type_   = Type::STRING;
  string_ = value;
}

/**
 * Raw string assignment operator
 *
 * @param value The value to be assigned
 * @return Reference to the current object
 */
Variant &Variant::operator=(char const *value)
{
  type_   = Type::STRING;
  string_ = ConstByteArray{value};

  return *this;
}

/**
 * Array index access operator
 *
 * @param index The index in the array to lookup
 * @return The reference to the variant stored at that index
 * @throws std::runtime_error if the variant doesn't exist
 */
Variant &Variant::operator[](std::size_t index)
{
  if (!IsArray())
  {
    throw std::runtime_error("Unable to access index of non-array variant");
  }

  return array_.at(index);
}

/**
 * Array index access operator
 *
 * @param index The index in the array to lookup
 * @return The reference to the variant stored at that index
 * @throws std::runtime_error if the variant doesn't exist
 */
Variant const &Variant::operator[](std::size_t index) const
{
  if (!IsArray())
  {
    throw std::runtime_error("Unable to access index of non-array variant");
  }

  return array_.at(index);
}

/**
 * Object access operator
 *
 * Creates or retrieves the currently stored value given a specified key
 *
 * @param key The key to lookup
 * @return The reference to the created or existing variant
 */
Variant &Variant::operator[](ConstByteArray const &key)
{
  if (!IsObject())
  {
    throw std::runtime_error("Unable to access keys of non-object variant");
  }

  auto it = object_.find(key);
  if (it != object_.end())
  {
    return *(it->second);
  }

  // allocate an element
  auto variant = std::make_unique<Variant>();

  // update the map
  object_.emplace(key, std::move(variant));

  // return the variant
  return *object_[key];
}

/**
 * Read only Object access operator
 *
 * @param key The key to lookup
 * @return The reference to the existing variant
 * @throws std::runtime_error when the variant is not an object and std::out_of_range when the key
 * is not present
 */
Variant const &Variant::operator[](ConstByteArray const &key) const
{
  if (!IsObject())
  {
    throw std::runtime_error("Unable to access keys of non-object variant");
  }

  auto it = object_.find(key);
  if (it == object_.end())
  {
    throw std::out_of_range("Key not present in object");
  }

  return *(it->second);
}

/**
 * Checks to see if a specified key is present in the object
 *
 * @param key The key to lookup
 * @return true if the key exists, otherwise false
 * @throws std::runtime_error if the variant is not an object
 */
bool Variant::Has(ConstByteArray const &key) const
{
  if (!IsObject())
  {
    throw std::runtime_error("Unable to access keys of non-object variant");
  }

  return object_.find(key) != object_.end();
}

/**
 * Calculates the size of the variant object.
 *
 * The size of the variant means different things depending on the type of the variant object:
 *
 * string - The length of the string
 * object - The number of keys present in the object
 * array - The number of elements in the array
 * otherwise - 0
 *
 * @return The calculated length
 */
std::size_t Variant::size() const
{
  std::size_t length{0};

  switch (type_)
  {
  case Type::UNDEFINED:
  case Type::INTEGER:
  case Type::FLOATING_POINT:
  case Type::FIXED_POINT:
  case Type::BOOLEAN:
  case Type::NULL_VALUE:
    break;

  case Type::STRING:
    length = string_.size();
    break;

  case Type::ARRAY:
    length = array_.size();
    break;

  case Type::OBJECT:
    length = object_.size();
    break;
  }

  return length;
}

/**
 * Update the size of the array to match the new given length
 *
 * When increasing the size of the array new elements are added to the end of the array and are
 * default initialised (i.e. they are "undefined")
 *
 * When removing elements from the array these two are also removed starting from the end of the
 * array
 *
 * @param length The new length of the array
 * @throws std::runtime_error if the variant it not an array
 */
void Variant::ResizeArray(std::size_t length)
{
  // ensure the at the element is of the correct type
  if (!IsArray())
  {
    throw std::runtime_error("Unable to resize non-array type");
  }

  // increase the array size
  array_.resize(length);
}

/**
 * Check inequality between two elements
 *
 * @param other The other variant to check against
 * @return true if not equal, otherwise false
 */
bool Variant::operator!=(Variant const &other) const
{
  return !operator==(other);
}

}  // namespace variant
}  // namespace fetch
