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

#include "meta/has_index.hpp"
#include "vectorise/platform.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
class UInt
{
public:
  using BaseType = uint8_t;
  using WideType = uint64_t;
  enum
  {
    UINT_SIZE         = S,
    ELEMENT_SIZE      = sizeof(BaseType) * 8,
    ELEMENTS          = UINT_SIZE / ELEMENT_SIZE,
    WIDE_ELEMENT_SIZE = sizeof(WideType) * 8,
    WIDE_ELEMENTS     = UINT_SIZE / WIDE_ELEMENT_SIZE
  };
  static_assert(S == (ELEMENTS * ELEMENT_SIZE),
                "Size must be a multiple of 8 times the base type size.");

  using WideContainerType                   = std::array<WideType, WIDE_ELEMENTS>;
  using ContainerType                       = std::array<BaseType, ELEMENTS>;
  static constexpr char const *LOGGING_NAME = "UInt";

  ////////////////////
  /// constructors ///
  ////////////////////

  constexpr UInt();
  constexpr UInt(UInt const &other);
  constexpr UInt(UInt &&other) noexcept;
  constexpr explicit UInt(ContainerType const &&other);
  constexpr explicit UInt(WideContainerType const &&other);
  template <typename T>
  constexpr UInt(T const &other, meta::IfIsAByteArray<T> * = nullptr);
  template <typename T>
  constexpr UInt(T const &other, meta::IfIsStdString<T> * = nullptr);
  template <typename T>
  constexpr UInt(T const &number, meta::IfIsInteger<T> * = nullptr);

  ////////////////////////////
  /// assignment operators ///
  ////////////////////////////

  constexpr UInt &operator=(UInt const &v);
  template <typename ArrayType>
  constexpr meta::HasIndex<ArrayType, UInt> &operator=(ArrayType const &v);
  template <typename T>
  constexpr meta::HasNoIndex<T, UInt> &operator=(T const &v);

  /////////////////////////////////////////////
  /// comparison operators for UInt objects ///
  /////////////////////////////////////////////

  constexpr bool operator==(UInt const &other) const;
  constexpr bool operator!=(UInt const &other) const;
  constexpr bool operator<(UInt const &o) const;
  constexpr bool operator>(UInt const &o) const;
  constexpr bool operator<=(UInt const &o) const;
  constexpr bool operator>=(UInt const &o) const;

  ////////////////////////////////////////////////
  /// comparison operators against other types ///
  ////////////////////////////////////////////////

  template <typename T>
  constexpr meta::IfIsInteger<T, bool> operator==(T const &o) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, bool> operator!=(T const &o) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, bool> operator<(T const &o) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, bool> operator>(T const &o) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, bool> operator<=(T const &o) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, bool> operator>=(T const &o) const;

  ///////////////////////
  /// unary operators ///
  ///////////////////////

  constexpr UInt &operator++();
  constexpr UInt &operator--();

  //////////////////////////////
  /// math and bit operators ///
  //////////////////////////////

  constexpr UInt  operator+(UInt const &n) const;
  constexpr UInt  operator-(UInt const &n) const;
  constexpr UInt  operator*(UInt const &n) const;
  constexpr UInt  operator/(UInt const &n) const;
  constexpr UInt  operator%(UInt const &n) const;
  constexpr UInt  operator&(UInt const &n) const;
  constexpr UInt  operator|(UInt const &n) const;
  constexpr UInt  operator^(UInt const &n) const;
  constexpr UInt &operator+=(UInt const &n);
  constexpr UInt &operator-=(UInt const &n);
  constexpr UInt &operator*=(UInt const &n);
  constexpr UInt &operator/=(UInt const &n);
  constexpr UInt &operator%=(UInt const &n);
  constexpr UInt &operator&=(UInt const &n);
  constexpr UInt &operator|=(UInt const &n);
  constexpr UInt &operator^=(UInt const &n);
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator+(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator-(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator*(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator/(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator%(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator&(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator|(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator^(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> &operator+=(T const &n);
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> &operator-=(T const &n);
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> &operator*=(T const &n);
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> &operator/=(T const &n);
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> &operator%=(T const &n);
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> &operator&=(T const &n);
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> &operator|=(T const &n);
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> &operator^=(T const &n);

  constexpr UInt &operator<<=(std::size_t const &n);
  constexpr UInt &operator>>=(std::size_t const &n);

  constexpr std::size_t msb() const;
  constexpr std::size_t lsb() const;

  /////////////////////////
  /// element accessors ///
  /////////////////////////

  constexpr uint8_t   operator[](std::size_t const &n) const;
  constexpr uint8_t & operator[](std::size_t const &n);
  constexpr WideType  elementAt(std::size_t const &n) const;
  constexpr WideType &elementAt(std::size_t const &n);

  constexpr uint64_t TrimmedSize() const;
  constexpr uint64_t size() const;

  constexpr uint8_t const *pointer() const;

  template <typename T, uint16_t G>
  constexpr friend void Serialize(T &s, UInt<G> const &u);

  template <typename T, uint16_t G>
  constexpr friend void Deserialize(T &s, UInt<G> &u);

  explicit operator std::string() const;

  /////////////////
  /// constants ///
  /////////////////

  static const UInt _0;
  static const UInt _1;
  static const UInt max;

private:
  union
  {
    ContainerType     base;
    WideContainerType wide;
  } data_;
};

/////////////////
/// constants ///
/////////////////

template <uint16_t S>
const UInt<S> UInt<S>::_0{0};
template <uint16_t S>
const UInt<S> UInt<S>::_1{1};
template <uint16_t S>
const UInt<S> UInt<S>::max{{ULONG_MAX, ULONG_MAX, ULONG_MAX, ULONG_MAX}};

////////////////////
/// constructors ///
////////////////////

template <uint16_t S>
constexpr UInt<S>::UInt()
{
  std::fill(data_.wide.begin(), data_.wide.end(), 0);
}

template <uint16_t S>
constexpr UInt<S>::UInt(UInt const &other)
  : data_{other.data_}
{}

template <uint16_t S>
constexpr UInt<S>::UInt(UInt &&other) noexcept
  : data_{std::move(other.data_)}
{}

template <uint16_t S>
constexpr UInt<S>::UInt(ContainerType const &&other)
  : data_(std::move(other))
{}

template <uint16_t S>
constexpr UInt<S>::UInt(WideContainerType const &&other)
  : data_(std::move(other))
{}

template <uint16_t S>
template <typename T>
constexpr UInt<S>::UInt(T const &other, meta::IfIsAByteArray<T> *)
{
  std::fill(data_.wide.begin(), data_.wide.end(), 0);
  std::copy(other.pointer(), other.pointer() + other.size(), data_.base.begin());
}

template <uint16_t S>
template <typename T>
constexpr UInt<S>::UInt(T const &other, meta::IfIsStdString<T> *)
{
  std::fill(data_.wide.begin(), data_.wide.end(), 0);
  std::copy(other.begin(), other.end(), data_.base.begin());
}

template <uint16_t S>
template <typename T>
constexpr UInt<S>::UInt(T const &number, meta::IfIsInteger<T> *)
{
  std::fill(data_.wide.begin(), data_.wide.end(), 0);
  data_.wide[0] = static_cast<uint64_t>(number);
}

////////////////////////////
/// assignment operators ///
////////////////////////////

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator=(UInt const &v)
{
  data_.wide = v.data_.wide;
  return *this;
}

template <uint16_t S>
template <typename ArrayType>
constexpr meta::HasIndex<ArrayType, UInt<S>> &UInt<S>::operator=(ArrayType const &v)
{
  std::fill(data_.wide.begin(), data_.wide.end(), 0);
  std::copy(v.pointer(), v.pointer() + v.capacity(), data_.base.begin());

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::HasNoIndex<T, UInt<S>> &UInt<S>::operator=(T const &v)
{
  std::fill(data_.wide.begin(), data_.wide.end(), 0);
  data_.wide[0] = static_cast<WideType>(v);

  return *this;
}

/////////////////////////////////////////////
/// comparison operators for UInt objects ///
/////////////////////////////////////////////

template <uint16_t S>
constexpr bool UInt<S>::operator==(UInt const &other) const
{
  bool ret = true;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    ret &= (data_.wide[i] == other.elementAt(i));
  }

  return ret;
}

template <uint16_t S>
constexpr bool UInt<S>::operator!=(UInt const &other) const
{
  return !(*this == other);
}

template <uint16_t S>
constexpr bool UInt<S>::operator<(UInt const &other) const
{
  // Simplified version, as we're dealing with wider elements
  for (std::size_t i = WIDE_ELEMENTS - 1; i >= 0; --i)
  {
    if (data_.wide[i] == other.elementAt(i))
    {
      continue;
    }
    else
    {
      return data_.wide[i] < other.elementAt(i);
    }
  }
  return false;
}

template <uint16_t S>
constexpr bool UInt<S>::operator>(UInt const &other) const
{
  return other < (*this);
}

template <uint16_t S>
constexpr bool UInt<S>::operator<=(UInt const &other) const
{
  return (*this == other) || (*this < other);
}

template <uint16_t S>
constexpr bool UInt<S>::operator>=(UInt const &other) const
{
  return (*this == other) || (*this > other);
}

////////////////////////////////////////////////
/// comparison operators against other types ///
////////////////////////////////////////////////

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, bool> UInt<S>::operator==(T const &other) const
{
  return (*this == UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, bool> UInt<S>::operator!=(T const &other) const
{
  return (*this != UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, bool> UInt<S>::operator<(T const &other) const
{
  return (*this < UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, bool> UInt<S>::operator>(T const &other) const
{
  return (*this > UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, bool> UInt<S>::operator<=(T const &other) const
{
  return (*this <= UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, bool> UInt<S>::operator>=(T const &other) const
{
  return (*this >= UInt<S>(other));
}

///////////////////////
/// unary operators ///
///////////////////////

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator++()
{
  *this += _1;

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator--()
{
  *this -= _1;

  return *this;
}

//////////////////////////////
/// math and bit operators ///
//////////////////////////////

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator+(UInt<S> const &n) const
{
  UInt<S> ret{*this};
  ret += n;

  return std::move(ret);
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator-(UInt<S> const &n) const
{
  UInt<S> ret{*this};
  ret -= n;

  return std::move(ret);
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator*(UInt<S> const &n) const
{
  UInt<S> ret{*this};
  ret *= n;

  return std::move(ret);
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator/(UInt<S> const &n) const
{
  UInt<S> ret{*this};
  ret /= n;

  return std::move(ret);
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator%(UInt<S> const &n) const
{
  UInt<S> ret{*this};
  ret %= n;

  return std::move(ret);
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator&(UInt<S> const &n) const
{
  UInt<S> ret{*this};
  ret &= n;

  return std::move(ret);
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator|(UInt<S> const &n) const
{
  UInt<S> ret{*this};
  ret |= n;

  return std::move(ret);
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator^(UInt<S> const &n) const
{
  UInt<S> ret{*this};
  ret ^= n;

  return std::move(ret);
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator+=(UInt<S> const &n)
{
  UInt<S>::WideType carry = 0, new_carry = 0;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    // if sum of elements is smaller than the element itself, then we have overflow and carry
    new_carry = (data_.wide[i] + n.elementAt(i) + carry < data_.wide[i]) ? 1 : 0;
    data_.wide[i] += n.elementAt(i) + carry;
    carry = new_carry;
  }

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator-=(UInt<S> const &n)
{
  if (*this < n)
  {
    *this = _0;
  }
  UInt<S>::WideType carry = 0, new_carry = 0;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    // if diff of the elements is larger than the element itself, then we have underflow and carry
    new_carry = (data_.wide[i] - n.elementAt(i) - carry > data_.wide[i]) ? 1 : 0;
    data_.wide[i] -= n.elementAt(i) + carry;
    carry = new_carry;
  }

  return *this;
}

// Implementation for 256-bits only
template <>
constexpr UInt<256> &UInt<256>::operator*=(UInt<256> const &n)
{
  // Calculate all products between each uint64_t element in the UInts
  // Use int128_t type to hold the actual product.
  __uint128_t products[WIDE_ELEMENTS][WIDE_ELEMENTS] = {};

  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    for (std::size_t j = 0; j < WIDE_ELEMENTS; ++j)
    {
      products[i][j] =
          static_cast<__uint128_t>(data_.wide[i]) * static_cast<__uint128_t>(n.elementAt(j));
    }
  }

  /* If a, b, two UInt<256> numbers, with the following elements (little endian order):
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

  __uint128_t carry = 0, terms[WIDE_ELEMENTS] = {};
  terms[0] = products[0][0];
  carry    = static_cast<WideType>(terms[0] >> WIDE_ELEMENT_SIZE);
  terms[1] = products[0][1] + products[1][0] + carry;
  carry    = static_cast<WideType>(terms[1] >> WIDE_ELEMENT_SIZE);
  terms[2] = products[0][2] + products[1][1] + products[2][0] + carry;
  carry    = static_cast<WideType>(terms[2] >> WIDE_ELEMENT_SIZE);
  terms[3] = products[0][3] + products[1][2] + products[2][1] + products[3][0] + carry;
  carry    = static_cast<WideType>(terms[3] >> WIDE_ELEMENT_SIZE);
  // TODO: decide what to do with overflow if carry > 0

  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    data_.wide[i] = static_cast<WideType>(terms[i]);
  }

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator/=(UInt<S> const &n)
{
  if (n == _0)
  {
    throw std::runtime_error("division by zero!");
  }
  if (n == _1)
  {
    *this = n;
    return *this;
  }
  if (*this == n)
  {
    *this = _1;
    return *this;
  }
  if (*this == _0)
  {
    *this = _0;
    return *this;
  }
  // No fractions supported, if *this < n return 0
  if (*this < _0)
  {
    *this = _0;
    return *this;
  }

  // Actual integer division algorithm
  // First simplify dividend/divisor by shifting right by min LSB bit
  // Essentially divide both by 2 as long as the LSB is zero
  UInt<S>     N{*this}, D{n};
  std::size_t lsb = std::min(N.lsb(), D.lsb());
  N >>= lsb;
  D >>= lsb;
  UInt<S> multiple = 1;

  // Find smallest multiple of divisor (D) that is larger than the dividend (N)
  while (N > D)
  {
    D <<= 1;
    multiple <<= 1;
  }
  // Calculate Quotient in a loop, essentially divide divisor by 2 and subtract from Remainder
  // Add multiple to Quotient
  UInt<S> Q = _0, R = N;
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
  *this = Q;
  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator%=(UInt<S> const &n)
{
  UInt<S> q = *this / n;
  *this -= q * n;
  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator&=(UInt<S> const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    data_.wide[i] &= n.elementAt(i);
  }

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator|=(UInt<S> const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    data_.wide[i] |= n.elementAt(i);
  }

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator^=(UInt<S> const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    data_.wide[i] ^= n.elementAt(i);
  }

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> UInt<S>::operator+(T const &n) const
{
  UInt<S> ret{*this}, nint{n};
  ret += nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> UInt<S>::operator-(T const &n) const
{
  UInt<S> ret{*this}, nint{n};
  ret -= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> UInt<S>::operator*(T const &n) const
{
  UInt<S> ret{*this}, nint{n};
  ret *= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> UInt<S>::operator/(T const &n) const
{
  UInt<S> ret{*this}, nint{n};
  ret /= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> UInt<S>::operator%(T const &n) const
{
  UInt<S> ret{*this}, nint{n};
  ret %= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> UInt<S>::operator&(T const &n) const
{
  UInt<S> ret{*this}, nint{n};
  ret &= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> UInt<S>::operator|(T const &n) const
{
  UInt<S> ret{*this}, nint{n};
  ret |= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> UInt<S>::operator^(T const &n) const
{
  UInt<S> ret{*this}, nint{n};
  ret ^= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> &UInt<S>::operator+=(T const &n)
{
  UInt<S> nint{n};
  *this += nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> &UInt<S>::operator-=(T const &n)
{
  UInt<S> nint{n};
  *this -= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> &UInt<S>::operator*=(T const &n)
{
  UInt<S> nint{n};
  *this *= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> &UInt<S>::operator/=(T const &n)
{
  UInt<S> nint{n};
  *this /= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> &UInt<S>::operator%=(T const &n)
{
  UInt<S> nint{n};
  *this %= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> &UInt<S>::operator&=(T const &n)
{
  UInt<S> nint{n};
  *this &= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> &UInt<S>::operator|=(T const &n)
{
  UInt<S> nint{n};
  *this |= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsInteger<T, UInt<S>> &UInt<S>::operator^=(T const &n)
{
  UInt<S> nint{n};
  *this ^= nint;

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator<<=(std::size_t const &bits)
{
  std::size_t full_words = bits / (sizeof(uint64_t) * 8);
  std::size_t real_bits  = bits - full_words * sizeof(uint64_t) * 8;
  std::size_t nbits      = WIDE_ELEMENT_SIZE - real_bits;
  // No actual shifting involved, just move the elements
  if (full_words)
  {
    for (std::size_t i = WIDE_ELEMENTS - 1; i >= full_words; i--)
    {
      data_.wide[i] = data_.wide[i - full_words];
    }
    for (std::size_t i = 0; i < full_words; i++)
    {
      data_.wide[i] = 0;
    }
  }
  // If real_bits == 0, nothing to do
  if (real_bits)
  {
    WideType carry = 0;
    for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
    {
      WideType val  = data_.wide[i];
      data_.wide[i] = (val << real_bits) | carry;
      carry         = val >> nbits;
    }
  }

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator>>=(std::size_t const &bits)
{
  std::size_t full_words = bits / (sizeof(uint64_t) * 8);
  std::size_t real_bits  = bits - full_words * sizeof(uint64_t) * 8;
  std::size_t nbits      = WIDE_ELEMENT_SIZE - real_bits;
  // No actual shifting involved, just move the elements
  if (full_words)
  {
    for (std::size_t i = 0; i < WIDE_ELEMENTS - full_words; i++)
    {
      data_.wide[i] = data_.wide[i + full_words];
    }
    for (std::size_t i = 0; i < full_words; i++)
    {
      data_.wide[WIDE_ELEMENTS - i - 1] = 0;
    }
  }

  // If real_bits == 0, nothing to do
  if (real_bits)
  {
    WideType carry = 0;
    for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
    {
      WideType val                      = data_.wide[WIDE_ELEMENTS - 1 - i];
      data_.wide[WIDE_ELEMENTS - 1 - i] = (val >> real_bits) | carry;
      carry                             = val << nbits;
    }
  }

  return *this;
}

template <uint16_t S>
constexpr std::size_t UInt<S>::msb() const
{
  std::size_t msb = 0;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
  {
    std::size_t msbi = platform::CountLeadingZeroes64(data_.wide[WIDE_ELEMENTS - 1 - i]);
    msb += msbi;
    if (msbi < 64)
    {
      return msb;
    }
  }
  return msb;
}

template <uint16_t S>
constexpr std::size_t UInt<S>::lsb() const
{
  std::size_t lsb = 0;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
  {
    std::size_t lsbi = platform::CountTrailingZeroes64(data_.wide[i]);
    lsb += lsbi;
    if (lsb < 64)
    {
      return lsb;
    }
  }
  return lsb;
}

/////////////////////////
/// element accessors ///
/////////////////////////

template <uint16_t S>
constexpr uint8_t UInt<S>::operator[](std::size_t const &n) const
{
  return data_.base[n];
}

template <uint16_t S>
constexpr uint8_t &UInt<S>::operator[](std::size_t const &n)
{
  return data_.base[n];
}

template <uint16_t S>
constexpr typename UInt<S>::WideType UInt<S>::elementAt(std::size_t const &n) const
{
  return data_.wide[n];
}

template <uint16_t S>
constexpr typename UInt<S>::WideType &UInt<S>::elementAt(std::size_t const &n)
{
  return data_.wide[n];
}

template <uint16_t S>
constexpr uint64_t UInt<S>::TrimmedSize() const
{
  uint64_t ret = WIDE_ELEMENTS;
  while ((ret != 0) && (data_.wide[ret - 1] == 0))
  {
    --ret;
  }
  return ret;
}

template <uint16_t S>
constexpr uint8_t const *UInt<S>::pointer() const
{
  return data_.base.data();
}

template <uint16_t S>
constexpr uint64_t UInt<S>::size() const
{
  return ELEMENTS;
}

template <uint16_t S>
UInt<S>::operator std::string() const
{
  std::stringstream ret;
  ret << std::hex;
  ret.fill('0');
  for (std::size_t i = 0; i < ELEMENTS; ++i)
  {
    ret << std::setw(2) << static_cast<uint16_t>(data_.base[ELEMENTS - i - 1]);
  }

  return ret.str();
}

inline double Log(UInt<256> const &x)
{
  uint64_t last_word = x.elementAt(x.TrimmedSize() - 1);

  uint64_t tz       = uint64_t(platform::CountTrailingZeroes64(last_word));
  uint64_t exponent = (last_word << 3) - tz;

  return double(exponent) + std::log(double(last_word << tz) * (1. / double(uint32_t(-1))));
}

inline double ToDouble(UInt<256> const &x)
{
  uint64_t last_word = x.elementAt(x.TrimmedSize() - 1);

  auto tz       = static_cast<uint16_t>(platform::CountTrailingZeroes64(last_word));
  auto exponent = static_cast<uint16_t>((last_word << 3u) - tz);

  assert(exponent < 1023);

  union
  {
    double   value;
    uint64_t bits;
  } conv;

  conv.bits = 0;
  conv.bits |= uint64_t(uint64_t(exponent & ((1u << 12u) - 1)) << 52u);
  conv.bits |= uint64_t((last_word << (20u + tz)) & ((1ull << 53u) - 1));
  return conv.value;
}

template <typename T, uint16_t S>
constexpr void Serialize(T &s, UInt<S> const &u)
{
  s << u.data_.base;
}

template <typename T, uint16_t S>
constexpr void Deserialize(T &s, UInt<S> &u)
{
  s >> u.data_.base;
}

}  // namespace vectorise
}  // namespace fetch
