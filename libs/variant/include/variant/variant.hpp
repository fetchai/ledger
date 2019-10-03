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

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"
#include "meta/type_traits.hpp"
#include "variant/detail/element_pool.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/meta/math_type_traits.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fetch {
namespace variant {

/**
 * The variant is a basic object that can store any of the following data types:
 *
 * - null
 * - boolean
 * - integer
 * - floating point
 * - array
 * - object
 *
 * It is a useful interchange format to JSON, YAML, Message Pack etc. Including other programming
 * languages.
 */
class Variant
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  enum class Type
  {
    UNDEFINED,
    INTEGER,
    FLOATING_POINT,
    FIXED_POINT,
    BOOLEAN,
    STRING,
    NULL_VALUE,
    ARRAY,
    OBJECT,
  };

  /// @name Non Value Helpers
  /// @{
  static Variant Null();
  static Variant Undefined();
  static Variant Array(std::size_t elements);
  static Variant Object();
  /// @}

  // Construction / Destruction
  Variant() = default;
  Variant(Variant const &other);
  Variant(Variant &&) noexcept = default;
  ~Variant()                   = default;

  template <typename T>
  explicit Variant(T const &value, meta::IfIsBoolean<T> * /*unused*/ = nullptr);
  template <typename T>
  explicit Variant(T const &value, meta::IfIsInteger<T> * /*unused*/ = nullptr);
  template <typename T>
  explicit Variant(T const &value, meta::IfIsFloat<T> * /*unused*/ = nullptr);
  template <typename T>
  explicit Variant(T const &value, math::meta::IfIsFixedPoint<T> * /*unused*/ = nullptr);
  template <typename T>
  explicit Variant(T &&value, meta::IfIsString<T> * /*unused*/ = nullptr);
  explicit Variant(char const *value);

  /// @name Basic Type Access
  /// @{
  Type type() const;

  bool IsUndefined() const;
  bool IsInteger() const;
  bool IsFloatingPoint() const;
  bool IsFixedPoint() const;
  bool IsBoolean() const;
  bool IsString() const;
  bool IsNull() const;
  bool IsArray() const;
  bool IsObject() const;
  /// @}

  /// @name Type Checking
  /// @{
  template <typename T>
  meta::IfIsBoolean<T, bool> Is() const;
  template <typename T>
  meta::IfIsInteger<T, bool> Is() const;
  template <typename T>
  meta::IfIsFloat<T, bool> Is() const;
  template <typename T>
  math::meta::IfIsFixedPoint<T, bool> Is() const;
  template <typename T>
  meta::IfIsString<T, bool> Is() const;
  /// @}

  /// @name Element Access
  /// @{
  template <typename T>
  meta::IfIsBoolean<T, T> As() const;
  template <typename T>
  meta::IfIsInteger<T, T> As() const;
  template <typename T>
  meta::IfIsFloat<T, T> As() const;
  template <typename T>
  math::meta::IfIsFixedPoint<T, T> As() const;
  template <typename T>
  meta::IfIsConstByteArray<T, ConstByteArray const &> As() const;
  template <typename T>
  meta::IfIsStdString<T, std::string> As() const;
  /// @}

  std::size_t size() const;

  /// @name Array Element Access
  /// @{
  Variant &      operator[](std::size_t index);
  Variant const &operator[](std::size_t index) const;
  void           ResizeArray(std::size_t length);
  /// @}

  /// @name Object Element Access
  /// @{
  Variant &      operator[](ConstByteArray const &key);
  Variant const &operator[](ConstByteArray const &key) const;
  bool           Has(ConstByteArray const &key) const;
  /// @}

  /// @name Operators
  /// @{
  template <typename T>
  meta::IfIsBoolean<T, Variant &> operator=(T const &value);  // NOLINT
  template <typename T>
  meta::IfIsInteger<T, Variant &> operator=(T const &value);  // NOLINT
  template <typename T>
  meta::IfIsFloat<T, Variant &> operator=(T const &value);  // NOLINT
  template <typename T>
  math::meta::IfIsFixedPoint<T, Variant &> operator=(T const &value);  // NOLINT
  template <typename T>
  meta::IfIsAByteArray<T, Variant &> operator=(T const &value);  // NOLINT
  template <typename T>
  meta::IfIsStdString<T, Variant &> operator=(T const &value);  // NOLINT
  Variant &                         operator=(char const *value);

  Variant &operator=(Variant const &value);

  bool operator==(Variant const &other) const;
  bool operator!=(Variant const &other) const;
  /// @}

  /// @name Iteration
  /// @{
  template <typename Function>
  void IterateObject(Function const &function) const;
  /// @}

  friend std::ostream &operator<<(std::ostream &stream, Variant const &variant);

private:
  using VariantList   = std::vector<Variant>;
  using VariantObject = std::unordered_map<ConstByteArray, std::unique_ptr<Variant>>;
  using Pool          = detail::ElementPool<Variant>;

  union PrimitiveData
  {
    int64_t integer;
    double  float_point;
    bool    boolean;
  };

  // Data Elements
  Type           type_{Type::UNDEFINED};  ///< The type of the variant
  PrimitiveData  primitive_{};            ///< Union of primitive data values
  ConstByteArray string_;                 ///< The string value of the variant
  VariantList    array_;                  ///< The array value of the variant
  VariantObject  object_;                 ///< The object value of the variant
};

/**
 * Boolean constructor
 *
 * @tparam T A boolean type
 * @param value The value to be set
 */
template <typename T>
Variant::Variant(T const &value, meta::IfIsBoolean<T> * /*unused*/)
  : Variant()
{
  type_              = Type::BOOLEAN;
  primitive_.boolean = value;
}

/**
 * Integer constructor
 *
 * @tparam T A integer based type
 * @param value The value to be set
 */
template <typename T>
Variant::Variant(T const &value, meta::IfIsInteger<T> * /*unused*/)
  : Variant()
{
  type_              = Type::INTEGER;
  primitive_.integer = static_cast<int64_t>(value);
}

/**
 * Floating point constructor
 *
 * @tparam T A floating point type
 * @param value The value to be set
 */
template <typename T>
Variant::Variant(T const &value, meta::IfIsFloat<T> * /*unused*/)
  : Variant()
{
  type_                  = Type::FLOATING_POINT;
  primitive_.float_point = static_cast<double>(value);
}

/**
 * Fixed point constructor
 *
 * @tparam T A fixed point type
 * @param value The value to be set
 */
template <typename T>
Variant::Variant(T const &value, math::meta::IfIsFixedPoint<T> * /*unused*/)
  : Variant()
{
  type_              = Type::FIXED_POINT;
  primitive_.integer = value.Data();
}

/**
 * String constructor
 *
 * @tparam T A string like object
 * @param value The value to be set
 */
template <typename T>
Variant::Variant(T &&value, meta::IfIsString<T> * /*unused*/)
  : Variant()
{
  type_   = Type::STRING;
  string_ = std::forward<T>(value);
}

/**
 * Boolean compatibility checker
 *
 * @tparam T A boolean type
 * @return true if the value is compatible boolean, otherwise false
 */
template <typename T>
meta::IfIsBoolean<T, bool> Variant::Is() const
{
  return type() == Type::BOOLEAN;
}

/**
 * Integer compatibility checker
 *
 * @tparam T A integer type
 * @return true if the value is compatible integer, otherwise false
 */
template <typename T>
meta::IfIsInteger<T, bool> Variant::Is() const
{
  return type() == Type::INTEGER;
}

/**
 * Float compatibility checker
 *
 * @tparam T A float type
 * @return true if the value is compatible float, otherwise false
 */
template <typename T>
meta::IfIsFloat<T, bool> Variant::Is() const
{
  return type() == Type::FLOATING_POINT;
}

/**
 * Fixedpoint compatibility checker
 *
 * @tparam T A fixed point type
 * @return true if the value is compatible fixed point, otherwise false
 */
template <typename T>
math::meta::IfIsFixedPoint<T, bool> Variant::Is() const
{
  return type() == Type::FIXED_POINT;
}

/**
 * String compatibility checker
 *
 * @tparam T A string type
 * @return true if the value is compatible string, otherwise false
 */
template <typename T>
meta::IfIsString<T, bool> Variant::Is() const
{
  return type() == Type::STRING;
}

/**
 * Boolean conversion accessor
 *
 * @tparam T A boolean type
 * @return The converted value
 * @throws std::runtime_error in the case where the conversion is not possible
 */
template <typename T>
meta::IfIsBoolean<T, T> Variant::As() const
{
  if (type() != Type::BOOLEAN)
  {
    throw std::runtime_error("Variant type mismatch, unable to extract boolean value");
  }

  return primitive_.boolean;
}

/**
 * Integer conversion accessor
 *
 * @tparam T A integer type
 * @return The converted value
 * @throws std::runtime_error in the case where the conversion is not possible
 */
template <typename T>
meta::IfIsInteger<T, T> Variant::As() const
{
  if (type() != Type::INTEGER)
  {
    throw std::runtime_error("Variant type mismatch, unable to extract integer value");
  }

  return static_cast<T>(primitive_.integer);
}

/**
 * Float conversion accessor
 *
 * @tparam T A float type
 * @return The converted value
 * @throws std::runtime_error in the case where the conversion is not possible
 */
template <typename T>
meta::IfIsFloat<T, T> Variant::As() const
{
  if (type() != Type::FLOATING_POINT)
  {
    throw std::runtime_error("Variant type mismatch, unable to extract floating point value");
  }

  return static_cast<T>(primitive_.float_point);
}

/**
 * Fixed point conversion accessor
 *
 * @tparam T A fixed point type
 * @return The converted value
 * @throws std::runtime_error in the case where the conversion is not possible
 */
template <typename T>
math::meta::IfIsFixedPoint<T, T> Variant::As() const
{
  if (type() != Type::FIXED_POINT)
  {
    throw std::runtime_error("Variant type mismatch, unable to extract floating point value");
  }

  return static_cast<T>(fixed_point::fp64_t::FromBase(primitive_.integer));
}

/**
 * ConstByteArray conversion accessor
 *
 * @tparam T A ConstByteArray type
 * @return The converted value
 * @throws std::runtime_error in the case where the conversion is not possible
 */
template <typename T>
meta::IfIsConstByteArray<T, Variant::ConstByteArray const &> Variant::As() const
{
  if (type() != Type::STRING)
  {
    throw std::runtime_error("Variant type mismatch, unable to extract string value");
  }

  return string_;
}

/**
 * std::string conversion accessor
 *
 * @tparam T A std::string type
 * @return The converted value
 * @throws std::runtime_error in the case where the conversion is not possible
 */
template <typename T>
meta::IfIsStdString<T, std::string> Variant::As() const
{
  if (type() != Type::STRING)
  {
    throw std::runtime_error("Variant type mismatch, unable to extract string value");
  }

  return static_cast<std::string>(string_);
}

/**
 * Boolean assignment operator
 *
 * @tparam T A boolean type
 * @param value The value to assign
 * @return The reference to the updated variant
 */
template <typename T>
meta::IfIsBoolean<T, Variant &> Variant::operator=(T const &value)  // NOLINT
{
  type_              = Type::BOOLEAN;
  primitive_.boolean = value;

  return *this;
}

/**
 * Integer assignment operator
 *
 * @tparam T An integer type
 * @param value The value to assign
 * @return The reference to the updated variant
 */
template <typename T>
meta::IfIsInteger<T, Variant &> Variant::operator=(T const &value)  // NOLINT
{
  type_              = Type::INTEGER;
  primitive_.integer = static_cast<int64_t>(value);

  return *this;
}

/**
 * Float assignment operator
 *
 * @tparam T A floating point type
 * @param value The value to assign
 * @return The reference to the updated variant
 */
template <typename T>
meta::IfIsFloat<T, Variant &> Variant::operator=(T const &value)  // NOLINT
{
  type_                  = Type::FLOATING_POINT;
  primitive_.float_point = static_cast<double>(value);

  return *this;
}

/**
 * Fixed point assignment operator
 *
 * @tparam T A fixed point type
 * @param value The value to assign
 * @return The reference to the updated variant
 */
template <typename T>
math::meta::IfIsFixedPoint<T, Variant &> Variant::operator=(T const &value)  // NOLINT
{
  type_              = Type::FIXED_POINT;
  primitive_.integer = value.Data();

  return *this;
}

/**
 * ByteArray assignment operator
 *
 * @tparam T A byte array type
 * @param value The value to assign
 * @return The reference to the updated variant
 */
template <typename T>
meta::IfIsAByteArray<T, Variant &> Variant::operator=(T const &value)  // NOLINT
{
  type_   = Type::STRING;
  string_ = value;

  return *this;
}

/**
 * std::string assignment operator
 *
 * @tparam T A std::string type
 * @param value The value to assign
 * @return The reference to the updated variant
 */
template <typename T>
meta::IfIsStdString<T, Variant &> Variant::operator=(T const &value)  // NOLINT
{
  type_   = Type::STRING;
  string_ = ConstByteArray{value};

  return *this;
}

/**
 * Iterates through items contained in variant Object (CONST version)
 *
 * @details This const CONST version of the @refitem(Variant::IterateObject)
 * method, thus it is NOT possible to modify value of items iterated through.
 *
 * @tparam Function - Please see desc. for the @refitem(Function) in the
 * @refitem(Variant::IterateObject) method. Since this is CONST version, passed
 * items of variant Object are IMMUTABLE, thus the implication is that signature
 * of the `Function` instance changes to: `bool(Variant const& item)`.
 *
 * @param function - Instance of functor, which will be called per each item
 * contained in the Variant Object, please see description of the the
 * @refitem(Function) for details.
 *
 * @return true if deserialisation passed successfully, false otherwise.
 */
template <typename Function>
void Variant::IterateObject(Function const &function) const
{
  if (IsObject())
  {
    for (auto const &item : object_)
    {
      if (!function(item.first, *item.second))
      {
        break;
      }
    }
  }
  else
  {
    throw std::runtime_error("Variant type mismatch, expected `object` type.");
  }
}

}  // namespace variant
}  // namespace fetch
