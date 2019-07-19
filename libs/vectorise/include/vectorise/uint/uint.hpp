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
    WIDE_ELEMENTS     = (UINT_SIZE + WIDE_ELEMENT_SIZE - 1) / WIDE_ELEMENT_SIZE
  };
  static_assert(S == (ELEMENTS * ELEMENT_SIZE),
                "Size must be a multiple of 8 times the base type size.");

  using WideContainerType                   = WideType[WIDE_ELEMENTS];
  using ContainerType                       = BaseType[ELEMENTS];
  static constexpr char const *LOGGING_NAME = "UInt";

  ////////////////////
  /// constructors ///
  ////////////////////

  constexpr UInt();
  constexpr UInt(UInt const &other)     = default;
  constexpr UInt(UInt &&other) noexcept = default;
  constexpr explicit UInt(ContainerType other);
  constexpr explicit UInt(WideContainerType other);
  template <typename T, meta::IfIsAByteArray<T> * = nullptr>
  UInt(T const &other);
  template <typename T, meta::IfIsUnsignedInteger<T> * = nullptr>
  constexpr UInt(T number);

  ////////////////////////////
  /// assignment operators ///
  ////////////////////////////

  constexpr UInt &operator=(UInt const &v);
  template <typename ArrayType>
  meta::IfHasIndex<ArrayType, UInt> &operator=(ArrayType const &v);
  template <typename T>
  constexpr meta::IfHasNoIndex<T, UInt> &operator=(T const &v);

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
  constexpr meta::IfIsUnsignedInteger<T, bool> operator==(T o) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator!=(T o) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator<(T o) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator>(T o) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator<=(T o) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator>=(T o) const;

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
  constexpr meta::IfIsUnsignedInteger<T, UInt> operator+(T n) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> operator-(T n) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> operator*(T n) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> operator/(T n) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> operator%(T n) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> operator&(T n) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> operator|(T n) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> operator^(T n) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> &operator+=(T n);
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> &operator-=(T n);
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> &operator*=(T n);
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> &operator/=(T n);
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> &operator%=(T n);
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> &operator&=(T n);
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> &operator|=(T n);
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, UInt> &operator^=(T n);

  constexpr UInt &operator<<=(std::size_t const &n);
  constexpr UInt &operator>>=(std::size_t const &n);

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
  WideContainerType wide_;

  constexpr ContainerType const &base() const;
  constexpr ContainerType &      base();
};

template <uint16_t S>
constexpr typename UInt<S>::ContainerType const &UInt<S>::base() const
{
  return reinterpret_cast<ContainerType const &>(wide_);
}

template <uint16_t S>
constexpr typename UInt<S>::ContainerType &UInt<S>::base()
{
  return reinterpret_cast<ContainerType &>(wide_);
}

/////////////////
/// constants ///
/////////////////

template <uint16_t S>
const UInt<S> UInt<S>::_0{0ull};
template <uint16_t S>
const UInt<S> UInt<S>::_1{1ull};
template <uint16_t S>
const UInt<S> UInt<S>::max{{ULONG_MAX, ULONG_MAX, ULONG_MAX, ULONG_MAX}};

////////////////////
/// constructors ///
////////////////////

template <uint16_t S>
constexpr UInt<S>::UInt()
{
  std::fill(wide_, wide_ + WIDE_ELEMENTS, 0);
}

template <uint16_t S>
constexpr UInt<S>::UInt(ContainerType other)
  : wide_{std::move(other)}
{}

template <uint16_t S>
constexpr UInt<S>::UInt(WideContainerType other)
  : wide_{std::move(other)}
{}

template <uint16_t S>
template <typename T, meta::IfIsAByteArray<T> *>
UInt<S>::UInt(T const &other)
{
  if (other.size() > ELEMENTS)
  {
    throw std::runtime_error("Size of input byte array is bigger than size of this UInt type.");
  }

  std::fill(wide_, wide_ + WIDE_ELEMENTS, 0);
  std::copy(other.pointer(), other.pointer() + other.size(), base());
}

template <uint16_t S>
template <typename T, meta::IfIsUnsignedInteger<T> *>
constexpr UInt<S>::UInt(T number)
{
  std::fill(wide_, wide_ + WIDE_ELEMENTS, 0);
  // This will work properly only on LITTLE endian hardware.
  reinterpret_cast<T &>(wide_[0]) = number;
}

////////////////////////////
/// assignment operators ///
////////////////////////////

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator=(UInt const &v)
{
  std::copy(v.pointer(), v.pointer() + ELEMENTS, base());
  return *this;
}

template <uint16_t S>
template <typename ArrayType>
meta::IfHasIndex<ArrayType, UInt<S>> &UInt<S>::operator=(ArrayType const &v)
{
  std::fill(wide_, wide_ + WIDE_ELEMENTS, 0);
  std::copy(v.pointer(), v.pointer() + v.capacity(), base());

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfHasNoIndex<T, UInt<S>> &UInt<S>::operator=(T const &v)
{
  std::fill(wide_, wide_ + WIDE_ELEMENTS, 0);
  wide_[0] = static_cast<WideType>(v);

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
    ret &= (wide_[i] == other.ElementAt(i));
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
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    if (wide_[WIDE_ELEMENTS - 1 - i] == other.ElementAt(WIDE_ELEMENTS - 1 - i))
    {
      continue;
    }
    else
    {
      return wide_[WIDE_ELEMENTS - 1 - i] < other.ElementAt(WIDE_ELEMENTS - 1 - i);
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
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator==(T other) const
{
  return (*this == UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator!=(T other) const
{
  return (*this != UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator<(T other) const
{
  return (*this < UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator>(T other) const
{
  return (*this > UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator<=(T other) const
{
  return (*this <= UInt<S>(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator>=(T other) const
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
    new_carry = (wide_[i] + n.ElementAt(i) + carry < wide_[i]) ? 1 : 0;
    wide_[i] += n.ElementAt(i) + carry;
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
    new_carry = (wide_[i] - n.ElementAt(i) - carry > wide_[i]) ? 1 : 0;
    wide_[i] -= n.ElementAt(i) + carry;
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
      // Note: C++14 does not have constexpr std::array, we need to cast the array
      products[i][j] =
          static_cast<__uint128_t>(wide_[i]) * static_cast<__uint128_t>(n.ElementAt(j));
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
    wide_[i] = static_cast<WideType>(terms[i]);
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
  UInt<S> multiple = 1u;

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
  } while (multiple != 0u);

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
    wide_[i] &= n.ElementAt(i);
  }

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator|=(UInt<S> const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    wide_[i] |= n.ElementAt(i);
  }

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator^=(UInt<S> const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    wide_[i] ^= n.ElementAt(i);
  }

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator+(T n) const
{
  UInt<S> ret{*this}, nint{n};
  ret += nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator-(T n) const
{
  UInt<S> ret{*this}, nint{n};
  ret -= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator*(T n) const
{
  UInt<S> ret{*this}, nint{n};
  ret *= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator/(T n) const
{
  UInt<S> ret{*this}, nint{n};
  ret /= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator%(T n) const
{
  UInt<S> ret{*this}, nint{n};
  ret %= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator&(T n) const
{
  UInt<S> ret{*this}, nint{n};
  ret &= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator|(T n) const
{
  UInt<S> ret{*this}, nint{n};
  ret |= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator^(T n) const
{
  UInt<S> ret{*this}, nint{n};
  ret ^= nint;

  return std::move(ret);
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator+=(T n)
{
  UInt<S> nint{n};
  *this += nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator-=(T n)
{
  UInt<S> nint{n};
  *this -= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator*=(T n)
{
  UInt<S> nint{n};
  *this *= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator/=(T n)
{
  UInt<S> nint{n};
  *this /= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator%=(T n)
{
  UInt<S> nint{n};
  *this %= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator&=(T n)
{
  UInt<S> nint{n};
  *this &= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator|=(T n)
{
  UInt<S> nint{n};
  *this |= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator^=(T n)
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
      wide_[i] = wide_[i - full_words];
    }
    for (std::size_t i = 0; i < full_words; i++)
    {
      wide_[i] = 0;
    }
  }
  // If real_bits == 0, nothing to do
  if (real_bits)
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
      wide_[i] = wide_[i + full_words];
    }
    for (std::size_t i = 0; i < full_words; i++)
    {
      wide_[WIDE_ELEMENTS - i - 1] = 0;
    }
  }

  // If real_bits == 0, nothing to do
  if (real_bits)
  {
    WideType carry = 0;
    for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
    {
      WideType val                 = wide_[WIDE_ELEMENTS - 1 - i];
      wide_[WIDE_ELEMENTS - 1 - i] = (val >> real_bits) | carry;
      carry                        = val << nbits;
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
constexpr std::size_t UInt<S>::lsb() const
{
  std::size_t lsb = 0;
  for (std::size_t i = 0; i < WIDE_ELEMENTS; i++)
  {
    std::size_t lsbi = platform::CountTrailingZeroes64(wide_[i]);
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
constexpr uint8_t UInt<S>::operator[](std::size_t n) const
{
  return base()[n];
}

template <uint16_t S>
constexpr uint8_t &UInt<S>::operator[](std::size_t n)
{
  return base()[n];
}

template <uint16_t S>
constexpr typename UInt<S>::WideType UInt<S>::ElementAt(std::size_t n) const
{
  return wide_[n];
}

template <uint16_t S>
constexpr typename UInt<S>::WideType &UInt<S>::ElementAt(std::size_t n)
{
  return wide_[n];
}

template <uint16_t S>
constexpr uint64_t UInt<S>::TrimmedSize() const
{
  uint64_t ret = WIDE_ELEMENTS;
  while ((ret != 0) && (wide_[ret - 1] == 0))
  {
    --ret;
  }
  return ret;
}

template <uint16_t S>
constexpr uint8_t const *UInt<S>::pointer() const
{
  return reinterpret_cast<uint8_t const *>(wide_);
}

template <uint16_t S>
constexpr uint64_t UInt<S>::size() const
{
  return ELEMENTS;
}

template <uint16_t S>
constexpr uint64_t UInt<S>::elements() const
{
  return WIDE_ELEMENTS;
}

template <uint16_t S>
UInt<S>::operator std::string() const
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

inline double Log(UInt<256> const &x)
{
  uint64_t last_word = x.ElementAt(x.TrimmedSize() - 1);

  uint64_t tz       = uint64_t(platform::CountTrailingZeroes64(last_word));
  uint64_t exponent = (last_word << 3) - tz;

  return double(exponent) + std::log(double(last_word << tz) * (1. / double(uint32_t(-1))));
}

inline double ToDouble(UInt<256> const &x)
{
  uint64_t last_word = x.ElementAt(x.TrimmedSize() - 1);

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
  for (std::size_t i = 0; i < u.elements(); i++)
  {
    s << u.ElementAt(i);
  }
}

template <typename T, uint16_t S>
constexpr void Deserialize(T &s, UInt<S> &u)
{
  for (std::size_t i = 0; i < u.elements(); i++)
  {
    s >> u.ElementAt(i);
  }
}

}  // namespace vectorise
}  // namespace fetch
