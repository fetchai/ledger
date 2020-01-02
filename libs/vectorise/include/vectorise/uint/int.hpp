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

#include "meta/has_index.hpp"
#include "meta/type_traits.hpp"
#include "vectorise/containers/array.hpp"
#include "vectorise/platform.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fetch {
namespace vectorise {

/* Implements a subset of big number functionality.
 *
 * The purpose of this library is to implement a subset of number
 * functionalities that is handy when for instance computing
 * proof-of-work or other big uint manipulations.
 *
 * The implementation subclasses a <byte_array::ConstByteArray> such
 * one easily use this in combination with hashes etc.
 */
template <uint16_t S = 256>
class Int
{
public:
  using BaseType = uint8_t;
  using WideType = uint64_t;
  enum
  {
    INT_SIZE          = S,
    ELEMENT_SIZE      = sizeof(BaseType) * 8,
    ELEMENTS          = INT_SIZE / ELEMENT_SIZE,
    WIDE_ELEMENT_SIZE = sizeof(WideType) * 8,
    WIDE_ELEMENTS     = (INT_SIZE + WIDE_ELEMENT_SIZE - 1) / WIDE_ELEMENT_SIZE,
  };
  static_assert(
      S >= 256,
      "Int class is intended for ints >= 256 bits, for smaller sizes, use the primitive types");
  static_assert(S == (WIDE_ELEMENTS * WIDE_ELEMENT_SIZE), "Size must be a multiple of 64 bits.");

  using WideContainerType                   = core::Array<WideType, WIDE_ELEMENTS>;
  using ContainerType                       = BaseType[ELEMENTS];
  static constexpr char const *LOGGING_NAME = "Int";

  template <typename... T>
  using IfIsWideInitialiserList = std::enable_if_t<meta::Is<WideType>::SameAsEvery<T...>::value &&
                                                   (sizeof...(T) <= WIDE_ELEMENTS)>;

  template <typename... T>
  using IfIsBaseInitialiserList =
      std::enable_if_t<meta::Is<BaseType>::SameAsEvery<T...>::value && (sizeof...(T) <= ELEMENTS)>;

  ////////////////////
  /// constructors ///
  ////////////////////

  constexpr Int()                     = default;
  constexpr Int(Int const &other)     = default;
  constexpr Int(Int &&other) noexcept = default;

  template <typename... T, IfIsWideInitialiserList<T...> * = nullptr>
  constexpr explicit Int(T &&... data)
    : wide_{{std::forward<T>(data)...}}
  {}

  template <typename... T, IfIsBaseInitialiserList<T...> * = nullptr>
  constexpr explicit Int(T &&... data)
    : wide_{reinterpret_cast<WideContainerType &&>(
          core::Array<BaseType, ELEMENTS>{{std::forward<T>(data)...}})}
  {}

  template <typename T, meta::IfIsAByteArray<T> * = nullptr>
  explicit Int(T const &other);
  template <typename T, meta::IfIsSignedInteger<T> * = nullptr>
  constexpr explicit Int(T const &number);
  template <typename T, meta::IfIsUnsignedInteger<T> * = nullptr>
  constexpr explicit Int(T const &number);
  constexpr explicit Int(int128_t const &number);
  constexpr explicit Int(uint128_t const &number);

  /////////////////////////
  /// casting operators ///
  /////////////////////////

  explicit operator std::string() const;
  template <typename T, meta::IfIsInteger<T> * = nullptr>
  explicit constexpr operator T() const;
  explicit constexpr operator int128_t() const;
  explicit constexpr operator uint128_t() const;

  ////////////////////////////
  /// assignment operators ///
  ////////////////////////////

  constexpr Int &operator=(Int const &v);
  template <typename ArrayType>
  meta::IfHasIndex<ArrayType, Int> &operator=(ArrayType const &v);  // NOLINT
  template <typename T>
  constexpr meta::IfHasNoIndex<T, Int> &operator=(T const &v);  // NOLINT

  /////////////////////////////////////////////
  /// comparison operators for Int objects ///
  /////////////////////////////////////////////

  constexpr bool operator==(Int const &other) const;
  constexpr bool operator!=(Int const &other) const;
  constexpr bool operator<(Int const &other) const;
  constexpr bool operator>(Int const &other) const;
  constexpr bool operator<=(Int const &other) const;
  constexpr bool operator>=(Int const &other) const;

  ////////////////////////////////////////////////
  /// comparison operators against other types ///
  ////////////////////////////////////////////////

  template <typename T>
  constexpr meta::IfIsSignedInteger<T, bool> operator==(T other) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, bool> operator!=(T other) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, bool> operator<(T other) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, bool> operator>(T other) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, bool> operator<=(T other) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, bool> operator>=(T other) const;

  ///////////////////////
  /// unary operators ///
  ///////////////////////

  constexpr Int &operator++();
  constexpr Int &operator--();
  constexpr Int  operator~() const;
  constexpr Int  operator+() const;
  constexpr Int  operator-() const;

  //////////////////////////////
  /// math and bit operators ///
  //////////////////////////////

  constexpr Int  operator+(Int const &n) const;
  constexpr Int  operator-(Int const &n) const;
  constexpr Int  operator*(Int const &n) const;
  constexpr Int  operator/(Int const &n) const;
  constexpr Int  operator%(Int const &n) const;
  constexpr Int  operator&(Int const &n) const;
  constexpr Int  operator|(Int const &n) const;
  constexpr Int  operator^(Int const &n) const;
  constexpr Int &operator+=(Int const &n);
  constexpr Int &operator-=(Int const &n);
  constexpr Int &operator*=(Int const &n);
  constexpr Int &operator/=(Int const &n);
  constexpr Int &operator%=(Int const &n);
  constexpr Int &operator&=(Int const &n);
  constexpr Int &operator|=(Int const &n);
  constexpr Int &operator^=(Int const &n);
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> operator+(T n) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> operator-(T n) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> operator*(T n) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> operator/(T n) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> operator%(T n) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> operator&(T n) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> operator|(T n) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> operator^(T n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, Int> operator<<(T n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, Int> operator>>(T n) const;
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> &operator+=(T n);
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> &operator-=(T n);
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> &operator*=(T n);
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> &operator/=(T n);
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> &operator%=(T n);
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> &operator&=(T n);
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> &operator|=(T n);
  template <typename T>
  constexpr meta::IfIsSignedInteger<T, Int> &operator^=(T n);
  template <typename T>
  constexpr meta::IfIsInteger<T, Int> &operator<<=(T n);
  template <typename T>
  constexpr meta::IfIsInteger<T, Int> &operator>>=(T n);

  constexpr Int         Sign() const;
  constexpr bool        IsPositive() const;
  constexpr std::size_t msb() const;
  constexpr std::size_t lsb() const;

  /////////////////////////
  /// element accessors ///
  /////////////////////////

  constexpr uint8_t   operator[](std::size_t n) const;
  constexpr uint8_t & operator[](std::size_t n);
  constexpr WideType  ElementAt(std::size_t n) const;
  constexpr WideType &ElementAt(std::size_t n);

  constexpr uint64_t TrimmedSize() const;
  constexpr uint64_t size() const;
  constexpr uint64_t elements() const;

  constexpr uint8_t const *pointer() const;

  template <typename T, uint16_t G>
  constexpr friend void Serialize(T &s, Int<G> const &u);

  template <typename T, uint16_t G>
  constexpr friend void Deserialize(T &s, Int<G> &u);

  /////////////////
  /// constants ///
  /////////////////

  static const Int _0;
  static const Int _1;
  static const Int max;
  static const Int min;

private:
  WideContainerType wide_;

  constexpr ContainerType const &base() const;
  constexpr ContainerType &      base();

  struct MaxValueConstructorEnabler
  {
  };
  constexpr explicit Int(MaxValueConstructorEnabler /*unused*/)
  {
    for (auto &itm : wide_)
    {
      itm = ~WideType{0};
    }
  }
};

template <uint16_t S>
inline std::ostream &operator<<(std::ostream &s, Int<S> const &x)
{
  s << std::string(x);
  return s;
}

template <uint16_t S>
constexpr typename Int<S>::ContainerType const &Int<S>::base() const
{
  return reinterpret_cast<ContainerType const &>(wide_.data());
}

template <uint16_t S>
constexpr typename Int<S>::ContainerType &Int<S>::base()
{
  return reinterpret_cast<ContainerType &>(wide_.data());
}

template <uint16_t S>
constexpr uint8_t const *Int<S>::pointer() const
{
  return reinterpret_cast<uint8_t const *>(wide_.data());
}

/////////////////////////
/// element accessors ///
/////////////////////////

template <uint16_t S>
constexpr uint8_t Int<S>::operator[](std::size_t n) const
{
  return base()[n];
}

template <uint16_t S>
constexpr uint8_t &Int<S>::operator[](std::size_t n)
{
  return base()[n];
}

template <uint16_t S>
constexpr typename Int<S>::WideType Int<S>::ElementAt(std::size_t n) const
{
  return wide_[n];
}

template <uint16_t S>
constexpr typename Int<S>::WideType &Int<S>::ElementAt(std::size_t n)
{
  return wide_[n];
}

/////////////////
/// constants ///
/////////////////

template <uint16_t S>
const Int<S> Int<S>::_0{0ull};
template <uint16_t S>
const Int<S> Int<S>::_1{1ull};
template <uint16_t S>
const Int<S> Int<S>::max{MaxValueConstructorEnabler{}};

////////////////////
/// constructors ///
////////////////////

template <uint16_t S>
template <typename T, meta::IfIsAByteArray<T> *>
Int<S>::Int(T const &other)
{
  if (other.size() > ELEMENTS)
  {
    throw std::runtime_error("Size of input byte array is bigger than size of this Int type.");
  }

  std::copy(other.pointer(), other.pointer() + other.size(), base());
  if (other < 0)
  {
    for (std::size_t i{other.size()}; i < WIDE_ELEMENTS; ++i)
    {
      wide_[i] = ~wide_[i];
    }
  }
}

template <uint16_t S>
template <typename T, meta::IfIsSignedInteger<T> *>
constexpr Int<S>::Int(T const &number)
{
  // This will work properly only on LITTLE endian hardware.
  auto *d                   = reinterpret_cast<char *>(wide_.data());
  *reinterpret_cast<T *>(d) = number;
  if (number < 0)
  {
    for (std::size_t i{1}; i < WIDE_ELEMENTS; ++i)
    {
      wide_[i] = ~wide_[i];
    }
  }
}

template <uint16_t S>
template <typename T, meta::IfIsUnsignedInteger<T> *>
constexpr Int<S>::Int(T const &number)
{
  // This will work properly only on LITTLE endian hardware.
  auto *d                   = reinterpret_cast<char *>(wide_.data());
  *reinterpret_cast<T *>(d) = number;
}

template <uint16_t S>
constexpr Int<S>::Int(int128_t const &number)
{
  wide_[0] = static_cast<WideType>(number);
  wide_[1] = static_cast<WideType>(number >> 64);
  if (number < 0)
  {
    for (std::size_t i{2}; i < WIDE_ELEMENTS; ++i)
    {
      wide_[i] = 0xffffffffffffffffULL;
    }
  }
}

template <uint16_t S>
constexpr Int<S>::Int(uint128_t const &number)
{
  wide_[0] = static_cast<WideType>(number);
  wide_[1] = static_cast<WideType>(number >> 64);
  for (std::size_t i{2}; i < WIDE_ELEMENTS; ++i)
  {
    wide_[i] = 0;
  }
}

/////////////////////////
/// casting operators ///
/////////////////////////

template <uint16_t S>
Int<S>::operator std::string() const
{
  std::stringstream ret;
  ret << std::hex;
  ret.fill('0');
  for (std::size_t i = 0; i < ELEMENTS; ++i)
  {
    ret << std::setw(2) << static_cast<uint16_t>(base()[ELEMENTS - i - 1]);
  }

  return ret.str();
}

template <uint16_t S>
template <typename T, meta::IfIsInteger<T> *>
constexpr Int<S>::operator T() const
{
  return static_cast<T>(wide_[0]);
}

template <uint16_t S>
constexpr Int<S>::operator int128_t() const
{
  int128_t x = wide_[0] | (static_cast<int128_t>(wide_[1]) << 64);
  return x;
}

template <uint16_t S>
constexpr Int<S>::operator uint128_t() const
{
  uint128_t x = wide_[0] | (static_cast<uint128_t>(wide_[1]) << 64);
  return x;
}

////////////////////////////
/// assignment operators ///
////////////////////////////

template <uint16_t S>
constexpr Int<S> &Int<S>::operator=(Int const &v)
{
  std::copy(v.pointer(), v.pointer() + ELEMENTS, base());
  return *this;
}

template <uint16_t S>  // NOLINT
template <typename ArrayType>
meta::IfHasIndex<ArrayType, Int<S>> &Int<S>::operator=(ArrayType const &v)  // NOLINT
{
  wide_.fill(0);
  std::copy(v.pointer(), v.pointer() + v.capacity(), base());

  return *this;
}

template <uint16_t S>  // NOLINT
template <typename T>
constexpr meta::IfHasNoIndex<T, Int<S>> &Int<S>::operator=(T const &v)  // NOLINT
{
  wide_.fill(0);
  wide_[0] = static_cast<WideType>(v);

  return *this;
}

/////////////////////////////////////////////
/// comparison operators for Int objects ///
/////////////////////////////////////////////

template <uint16_t S>
constexpr bool Int<S>::operator==(Int const &other) const
{
  bool ret = true;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    ret &= (wide_[i] == other.ElementAt(i));
  }

  return ret;
}

template <uint16_t S>
constexpr bool Int<S>::operator!=(Int const &other) const
{
  return !(*this == other);
}

template <uint16_t S>
constexpr bool Int<S>::operator<(Int const &other) const
{
  if (IsPositive() == other.IsPositive())
  {
    // Simplified version, as we're dealing with wider elements
    for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
    {
      if (wide_[WIDE_ELEMENTS - 1 - i] == other.ElementAt(WIDE_ELEMENTS - 1 - i))
      {
        continue;
      }

      return wide_[WIDE_ELEMENTS - 1 - i] < other.ElementAt(WIDE_ELEMENTS - 1 - i);
    }
  }
  else
  {
    // positive < negative is false and negative < positive is true
    return other.IsPositive();
  }
  return false;
}

template <uint16_t S>
constexpr bool Int<S>::operator>(Int const &other) const
{
  return other < (*this);
}

template <uint16_t S>
constexpr bool Int<S>::operator<=(Int const &other) const
{
  return (*this == other) || (*this < other);
}

template <uint16_t S>
constexpr bool Int<S>::operator>=(Int const &other) const
{
  return (*this == other) || (*this > other);
}

////////////////////////////////////////////////
/// comparison operators against other types ///
////////////////////////////////////////////////

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, bool> Int<S>::operator==(T other) const
{
  return (*this == Int<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, bool> Int<S>::operator!=(T other) const
{
  return (*this != Int<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, bool> Int<S>::operator<(T other) const
{
  return (*this < Int<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, bool> Int<S>::operator>(T other) const
{
  return (*this > Int<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, bool> Int<S>::operator<=(T other) const
{
  return (*this <= Int<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, bool> Int<S>::operator>=(T other) const
{
  return (*this >= Int<S>(other));
}

///////////////////////
/// unary operators ///
//////////////////////1/

template <uint16_t S>
constexpr Int<S> &Int<S>::operator++()
{
  *this += _1;

  return *this;
}

template <uint16_t S>
constexpr Int<S> &Int<S>::operator--()
{
  *this -= _1;

  return *this;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator~() const
{
  Int<S> retval;
  for (std::size_t i{0}; i < WIDE_ELEMENTS; ++i)
  {
    retval.wide_[i] = ~wide_[i];
  }

  return retval;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator+() const
{
  return *this;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator-() const
{
  Int<S> retval;
  for (std::size_t i{0}; i < WIDE_ELEMENTS; ++i)
  {
    retval.wide_[i] = ~wide_[i];
  }
  ++retval;

  return retval;
}

//////////////////////////////
/// math and bit operators ///
//////////////////////////////

template <uint16_t S>
constexpr Int<S> Int<S>::operator+(Int<S> const &n) const
{
  Int<S> ret{*this};
  ret += n;

  return ret;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator-(Int<S> const &n) const
{
  Int<S> ret{*this};
  ret -= n;

  return ret;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator*(Int<S> const &n) const
{
  Int<S> ret{*this};
  ret *= n;

  return ret;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator/(Int<S> const &n) const
{
  Int<S> ret{*this};
  ret /= n;

  return ret;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator%(Int<S> const &n) const
{
  Int<S> ret{*this};
  ret %= n;

  return ret;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator&(Int<S> const &n) const
{
  Int<S> ret{*this};
  ret &= n;

  return ret;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator|(Int<S> const &n) const
{
  Int<S> ret{*this};
  ret |= n;

  return ret;
}

template <uint16_t S>
constexpr Int<S> Int<S>::operator^(Int<S> const &n) const
{
  Int<S> ret{*this};
  ret ^= n;

  return ret;
}

template <uint16_t S>
constexpr Int<S> &Int<S>::operator+=(Int<S> const &n)
{
  if (n < _0)
  {
    *this -= -n;
    return *this;
  }
  Int<S>::WideType carry = 0, new_carry = 0;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    // if sum of elements is smaller than the element itself, then we have overflow and carry
    new_carry = (wide_[i] + n.ElementAt(i) + carry < wide_[i]) ? 1 : 0;
    wide_[i] += n.ElementAt(i) + carry;
    carry = new_carry;
  }

  return *this;
}

template <uint16_t S>
constexpr Int<S> &Int<S>::operator-=(Int<S> const &n)
{
  if (n < _0)
  {
    *this += -n;
    return *this;
  }
  Int<S>::WideType carry = 0, new_carry = 0;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    // if diff of the elements is larger than the element itself, then we have underflow and carry
    new_carry = (wide_[i] < n.ElementAt(i) + new_carry) ? 1 : 0;
    wide_[i] -= n.ElementAt(i) + carry;
    carry = new_carry;
  }

  return *this;
}

// Implementation for 256-bits only
// template <>
template <uint16_t S>
constexpr Int<S> &Int<S>::operator*=(Int<S> const &n)
{
  if (*this == _0 || n == _0)
  {
    *this = _0;
    return *this;
  }

  bool sign = true;
  if (*this < _0)
  {
    *this = -*this;
    sign  = !sign;
  }
  Int<256> o{n};
  if (o < _0)
  {
    o    = -o;
    sign = !sign;
  }

  // Calculate all products between each uint64_t element in the Ints
  // Use int128_t type to hold the actual product.
  uint128_t products[WIDE_ELEMENTS][WIDE_ELEMENTS] = {};

  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    for (std::size_t j = 0; j < WIDE_ELEMENTS; ++j)
    {
      // Note: C++14 does not have constexpr std::array, we need to cast the array
      products[i][j] = static_cast<uint128_t>(wide_[i]) * static_cast<uint128_t>(o.ElementAt(j));
    }
  }

  /* If a, b, two Int<256> numbers, with the following elements (little endian order):
   * |  a[3]  |  a[2]  |  a[1]  |  a[0]  |
   * |  b[3]  |  b[2]  |  b[1]  |  b[0]  |
   * Then both can be written in the following form:
   * a = a[0] + (a[1] << 64) + (a[2] << 128) + (a[3] << 192)
   * b = b[0] + (b[1] << 64) + (b[2] << 128) + (b[3] << 192)
   *
   * Then a * b is the following, truncating terms left shifted over 192 bits:
   * a * b =  a[0] * b[0]
   *       + (a[0] * b[1] + a[1] * b[0]) << 64
   *       + (a[0] * b[2] + a[1] * b[1] + a[2] * b[0]) << 128
   *       + (a[0] * b[3] + a[1] * b[2] + a[2] * b[1] + a[3] * b[0]) << 192
   * In order to be accurate, we calculate these terms as 128-bit quantities and we add the
   * high-64-bits as carry to the next term
   */

  uint128_t carry = 0, terms[WIDE_ELEMENTS] = {};
  terms[0] = products[0][0];
  carry    = static_cast<WideType>(terms[0] >> WIDE_ELEMENT_SIZE);
  terms[1] = products[0][1] + products[1][0] + carry;
  carry    = static_cast<WideType>(terms[1] >> WIDE_ELEMENT_SIZE);
  terms[2] = products[0][2] + products[1][1] + products[2][0] + carry;
  carry    = static_cast<WideType>(terms[2] >> WIDE_ELEMENT_SIZE);
  terms[3] = products[0][3] + products[1][2] + products[2][1] + products[3][0] + carry;

  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    wide_[i] = static_cast<WideType>(terms[i]);
  }

  if (!sign)
  {
    *this = -*this;
  }
  return *this;
}

template <uint16_t S>
constexpr Int<S> &Int<S>::operator/=(Int<S> const &n)
{
  if (n == _0)
  {
    throw std::runtime_error("division by zero!");
  }
  if (n == _1)
  {
    return *this;
  }
  if (*this == n)
  {
    *this = _1;
    return *this;
  }
  if (*this == _0)
  {
    return *this;
  }

  // Actual integer division algorithm
  // First simplify dividend/divisor by shifting right by min LSB bit
  // Essentially divide both by 2 as long as the LSB is zero
  Int<S> N{*this}, D{n};
  Int<S> sign = _1;
  if (N < _0)
  {
    N    = -N;
    sign = -sign;
  }
  if (D < _0)
  {
    D    = -D;
    sign = -sign;
  }

  std::size_t lsb = std::min(N.lsb(), D.lsb());
  N >>= lsb;
  D >>= lsb;
  Int<S> multiple(1u);

  // Find smallest multiple of divisor (D) that is larger than the dividend (N)
  while (N > D)
  {
    D <<= 1;
    multiple <<= 1;
  }
  // Calculate Quotient in a loop, essentially divide divisor by 2 and subtract from Remainder
  // Add multiple to Quotient
  Int<S> Q = _0, R = N;
  do
  {
    if (R >= D)
    {
      R -= D;
      Q += multiple;
    }
    D >>= 1;  // Divide by two.
    multiple >>= 1;
  } while (multiple != 0);

  // Return the Quotient
  *this = sign * Q;
  return *this;
}

template <uint16_t S>
constexpr Int<S> &Int<S>::operator%=(Int<S> const &n)
{
  Int<S> q = *this / n;
  *this -= q * n;
  return *this;
}

template <uint16_t S>
constexpr Int<S> &Int<S>::operator&=(Int<S> const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    wide_[i] &= n.ElementAt(i);
  }

  return *this;
}

template <uint16_t S>
constexpr Int<S> &Int<S>::operator|=(Int<S> const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    wide_[i] |= n.ElementAt(i);
  }

  return *this;
}

template <uint16_t S>
constexpr Int<S> &Int<S>::operator^=(Int<S> const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    wide_[i] ^= n.ElementAt(i);
  }

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> Int<S>::operator+(T n) const
{
  Int<S> ret{*this}, nint{n};
  ret += nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> Int<S>::operator-(T n) const
{
  Int<S> ret{*this}, nint{n};
  ret -= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> Int<S>::operator*(T n) const
{
  Int<S> ret{*this}, nint{n};
  ret *= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> Int<S>::operator/(T n) const
{
  Int<S> ret{*this}, nint{n};
  ret /= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> Int<S>::operator%(T n) const
{
  Int<S> ret{*this}, nint{n};
  ret %= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> Int<S>::operator&(T n) const
{
  Int<S> ret{*this}, nint{n};
  ret &= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> Int<S>::operator|(T n) const
{
  Int<S> ret{*this}, nint{n};
  ret |= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> Int<S>::operator^(T n) const
{
  Int<S> ret{*this}, nint{n};
  ret ^= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, Int<S>> Int<S>::operator<<(T n) const
{
  Int<S> ret{*this};
  ret <<= n;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, Int<S>> Int<S>::operator>>(T n) const
{
  Int<S> ret{*this};
  ret >>= n;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> &Int<S>::operator+=(T n)
{
  Int<S> nint{n};
  *this += nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> &Int<S>::operator-=(T n)
{
  Int<S> nint{n};
  *this -= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> &Int<S>::operator*=(T n)
{
  Int<S> nint{n};
  *this *= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> &Int<S>::operator/=(T n)
{
  Int<S> nint{n};
  *this /= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> &Int<S>::operator%=(T n)
{
  Int<S> nint{n};
  *this %= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> &Int<S>::operator&=(T n)
{
  Int<S> nint{n};
  *this &= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> &Int<S>::operator|=(T n)
{
  Int<S> nint{n};
  *this |= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsSignedInteger<T, Int<S>> &Int<S>::operator^=(T n)
{
  Int<S> nint{n};
  *this ^= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, Int<S>> &Int<S>::operator<<=(T n)
{
  std::size_t full_words = static_cast<std::size_t>(n) / (sizeof(uint64_t) * 8);
  std::size_t real_bits  = static_cast<std::size_t>(n) - full_words * sizeof(uint64_t) * 8;
  std::size_t nbits      = WIDE_ELEMENT_SIZE - real_bits;
  // No actual shifting involved, just move the elements
  if (full_words != 0u)
  {
    for (std::size_t i = WIDE_ELEMENTS - 1; i >= full_words; i--)
    {
      wide_[i] = wide_[i - full_words];
    }
    for (std::size_t i = 0; i < full_words; i++)
    {
      wide_[i] = 0;
    }
  }
  // If real_bits == 0, nothing to do
  if (real_bits != 0u)
  {
    WideType carry = 0;
    for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
    {
      WideType val = wide_[i];
      wide_[i]     = (val << real_bits) | carry;
      carry        = val >> nbits;
    }
  }

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, Int<S>> &Int<S>::operator>>=(T n)
{
  bool        is_positive = IsPositive();
  WideType    mask        = is_positive ? 0 : 0xffffffffffffffffULL;
  std::size_t full_words  = static_cast<std::size_t>(n) / (sizeof(uint64_t) * 8);
  std::size_t real_bits   = static_cast<std::size_t>(n) - full_words * sizeof(uint64_t) * 8;
  std::size_t nbits       = WIDE_ELEMENT_SIZE - real_bits;
  // No actual shifting involved, just move the elements
  if (full_words != 0u)
  {
    for (std::size_t i = 0; i < WIDE_ELEMENTS - full_words; i++)
    {
      wide_[i] = wide_[i + full_words];
    }
    for (std::size_t i = 0; i < full_words; i++)
    {
      wide_[WIDE_ELEMENTS - i - 1] = mask;
    }
  }

  if (is_positive)
  {
    // If real_bits == 0, nothing to do
    if (real_bits != 0u)
    {
      WideType carry = 0;
      for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
      {
        WideType val                 = wide_[WIDE_ELEMENTS - 1 - i];
        wide_[WIDE_ELEMENTS - 1 - i] = (val >> real_bits) | carry;
        carry                        = val << nbits;
      }
    }
  }

  return *this;
}

template <uint16_t S>
constexpr Int<S> Int<S>::Sign() const
{
  return IsPositive() ? Int<S>::_1 : -(Int<S>::_1);
}

template <uint16_t S>
constexpr bool Int<S>::IsPositive() const
{
  return ((wide_[WIDE_ELEMENTS - 1] & 0x8000000000000000ULL) >> 63) == 0;
}

template <uint16_t S>
constexpr std::size_t Int<S>::msb() const
{
  std::size_t msb = 0;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
  {
    std::size_t msbi = platform::CountLeadingZeroes64(wide_[WIDE_ELEMENTS - 1 - i]);
    msb += msbi;
    if (msbi < 64)
    {
      return msb;
    }
  }
  return msb;
}

template <uint16_t S>
constexpr std::size_t Int<S>::lsb() const
{
  std::size_t lsb = 0;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
  {
    std::size_t lsbi = platform::CountTrailingZeroes64(wide_[i]);
    lsb += lsbi;
    if (lsbi < 64)
    {
      return lsb;
    }
  }
  return lsb;
}

template <uint16_t S>
constexpr uint64_t Int<S>::TrimmedSize() const
{
  uint64_t ret = WIDE_ELEMENTS;
  while ((ret != 0) && (wide_[ret - 1] == 0))
  {
    --ret;
  }
  return ret;
}

template <uint16_t S>
constexpr uint64_t Int<S>::size() const
{
  return ELEMENTS;
}

template <uint16_t S>
constexpr uint64_t Int<S>::elements() const
{
  return WIDE_ELEMENTS;
}

}  // namespace vectorise

}  // namespace fetch
