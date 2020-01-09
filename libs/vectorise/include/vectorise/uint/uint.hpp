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

// TODO(issue 1383): Handle 'residual' bits in the last element of `wide_` array
template <uint16_t S = 256>
class UInt
{
public:
  using BaseType = uint8_t;
  using WideType = uint64_t;
  enum : uint64_t
  {
    UINT_SIZE         = S,
    ELEMENT_SIZE      = sizeof(BaseType) * 8,
    ELEMENTS          = UINT_SIZE / ELEMENT_SIZE,
    WIDE_ELEMENT_SIZE = sizeof(WideType) * 8,
    WIDE_ELEMENTS     = ((UINT_SIZE + WIDE_ELEMENT_SIZE) - 1) / WIDE_ELEMENT_SIZE,
    RESIDUAL_BITS     = WIDE_ELEMENTS * WIDE_ELEMENT_SIZE - UINT_SIZE,
  };
  static_assert(S == (ELEMENTS * ELEMENT_SIZE),
                "Size must be a multiple of 8 times the base type size.");

  using WideContainerType                   = core::Array<WideType, WIDE_ELEMENTS>;
  using ContainerType                       = BaseType[ELEMENTS];
  static constexpr char const *LOGGING_NAME = "UInt";

  template <typename... T>
  using IfIsWideInitialiserList =
      std::enable_if_t<meta::Is<WideType>::SameAsEvery<meta::Decay<T>...>::value &&
                       (sizeof...(T) <= WIDE_ELEMENTS)>;

  template <typename... T>
  using IfIsBaseInitialiserList =
      std::enable_if_t<meta::Is<BaseType>::SameAsEvery<meta::Decay<T>...>::value &&
                       (sizeof...(T) <= ELEMENTS)>;

  ////////////////////
  /// constructors ///
  ////////////////////

  constexpr UInt()                      = default;
  constexpr UInt(UInt const &other)     = default;
  constexpr UInt(UInt &&other) noexcept = default;

  template <typename... T, IfIsWideInitialiserList<T...> * = nullptr>
  constexpr explicit UInt(T &&... data)
    : wide_{{std::forward<T>(data)...}}
  {}

  template <typename... T, IfIsBaseInitialiserList<T...> * = nullptr>
  constexpr explicit UInt(T &&... data)
    : wide_{reinterpret_cast<WideContainerType &&>(
          core::Array<BaseType, ELEMENTS>{{std::forward<T>(data)...}})}
  {}

  template <typename T, meta::IfIsAByteArray<T> * = nullptr>
  constexpr explicit UInt(T const &other, platform::Endian endianess_of_input_data);
  template <typename T, meta::IfIsUnsignedInteger<T> * = nullptr>
  constexpr explicit UInt(T number);

  ////////////////////////////
  /// assignment operators ///
  ////////////////////////////

  constexpr UInt &operator=(UInt const &v);
  template <typename T>
  constexpr meta::IfHasNoIndex<T, UInt> &operator=(T const &v);  // NOLINT

  template <typename T>
  constexpr meta::IfIsAByteArray<T, UInt> &FromArray(
      T const &        arr,
      platform::Endian endianess_of_input_data);  // NOLINT

  /////////////////////////////////////////////
  /// comparison operators for UInt objects ///
  /////////////////////////////////////////////

  constexpr bool operator==(UInt const &other) const;
  constexpr bool operator!=(UInt const &other) const;
  constexpr bool operator<(UInt const &other) const;
  constexpr bool operator>(UInt const &other) const;
  constexpr bool operator<=(UInt const &other) const;
  constexpr bool operator>=(UInt const &other) const;

  ////////////////////////////////////////////////
  /// comparison operators against other types ///
  ////////////////////////////////////////////////

  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator==(T other) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator!=(T other) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator<(T other) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator>(T other) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator<=(T other) const;
  template <typename T>
  constexpr meta::IfIsUnsignedInteger<T, bool> operator>=(T other) const;

  ///////////////////////
  /// unary operators ///
  ///////////////////////

  constexpr UInt &operator++();
  constexpr UInt &operator--();
  constexpr UInt  operator+() const;
  constexpr UInt  operator~() const;

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

  constexpr UInt &operator<<=(std::size_t bits);
  constexpr UInt &operator>>=(std::size_t bits);

  constexpr std::size_t msb() const;
  constexpr std::size_t lsb() const;

  /////////////////////////
  /// element accessors ///
  /////////////////////////

  constexpr uint8_t  operator[](std::size_t n) const;
  constexpr uint8_t &operator[](std::size_t n);
  constexpr WideType ElementAt(std::size_t n) const
  {
    return wide_[n];
  }
  constexpr WideType &ElementAt(std::size_t n)
  {
    return wide_[n];
  }

  constexpr uint64_t TrimmedWideSize() const;
  constexpr uint64_t TrimmedSize() const;
  constexpr uint64_t size() const;
  constexpr uint64_t elements() const;

  constexpr uint8_t const *pointer() const;

  template <typename T, uint16_t G>
  constexpr friend void Serialize(T &s, UInt<G> const &u);

  template <typename T, uint16_t G>
  constexpr friend void Deserialize(T &s, UInt<G> &u);

  explicit operator std::string() const;

  template <typename T>
  constexpr meta::EnableIf<std::is_same<meta::Decay<T>, byte_array::ByteArray>::value, T> As(
      platform::Endian endianess_of_output_data, bool include_leading_zeroes = false) const;

  /////////////////
  /// constants ///
  /////////////////

  static const UInt _0;
  static const UInt _1;
  static const UInt max;

private:
  WideContainerType wide_;

  static constexpr WideType RESIDUAL_BITS_MASK{~WideType{0} >> RESIDUAL_BITS};

  constexpr ContainerType const &base() const;
  constexpr ContainerType &      base();

  template <uint16_t G = S>
  constexpr meta::EnableIf<(G % WIDE_ELEMENT_SIZE) == 0u, void> mask_residual_bits()
  {}

  template <uint16_t G = S>
  constexpr meta::EnableIf<(G % WIDE_ELEMENT_SIZE) != 0u, void> mask_residual_bits()
  {
    wide_[WIDE_ELEMENTS - 1] &= RESIDUAL_BITS_MASK;
  }

  template <typename T>
  constexpr meta::IfIsAByteArray<T> FromArrayInternal(T const &        arr,
                                                      platform::Endian endianess_of_input_data,
                                                      bool             zero_content);  // NOLINT

  struct MaxValueConstructorEnabler
  {
  };

  constexpr explicit UInt(MaxValueConstructorEnabler /*unused*/)
  {
    for (auto &itm : wide_)
    {
      itm = ~WideType{0};
    }
    mask_residual_bits();
  }
};

template <uint16_t S>
constexpr typename UInt<S>::ContainerType const &UInt<S>::base() const
{
  return reinterpret_cast<ContainerType const &>(wide_.data());
}

template <uint16_t S>
constexpr typename UInt<S>::ContainerType &UInt<S>::base()
{
  return reinterpret_cast<ContainerType &>(wide_.data());
}

/////////////////
/// constants ///
/////////////////

template <uint16_t S>
const UInt<S> UInt<S>::_0{0ull};
template <uint16_t S>
const UInt<S> UInt<S>::_1{1ull};
template <uint16_t S>
const UInt<S> UInt<S>::max{MaxValueConstructorEnabler{}};

////////////////////
/// constructors ///
////////////////////

template <uint16_t S>
template <typename T, meta::IfIsAByteArray<T> *>
constexpr UInt<S>::UInt(T const &other, platform::Endian endianess_of_input_data)
{
  FromArrayInternal(other, endianess_of_input_data, false);
}

template <uint16_t S>
template <typename T, meta::IfIsUnsignedInteger<T> *>
constexpr UInt<S>::UInt(T number)
{
  // This will work properly only on LITTLE endian hardware.
  auto *d                   = reinterpret_cast<char *>(wide_.data());
  *reinterpret_cast<T *>(d) = number;
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
template <typename T>
constexpr meta::IfIsAByteArray<T, UInt<S>> &UInt<S>::FromArray(
    T const &arr, platform::Endian endianess_of_input_data)  // NOLINT
{
  FromArrayInternal(arr, endianess_of_input_data, true);
  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsAByteArray<T> UInt<S>::FromArrayInternal(
    T const &arr, platform::Endian endianess_of_input_data,
    bool zero_content)  // NOLINT
{
  auto const size{arr.size()};

  if (0 == size)
  {
    if (zero_content)
    {
      wide_.fill(0);
    }
    return;
  }

  if (size > ELEMENTS)
  {
    throw std::runtime_error("Size of input byte array is bigger than size of this UInt type.");
  }

  if (zero_content)
  {
    wide_.fill(0);
  }

  switch (endianess_of_input_data)
  {
  case platform::Endian::LITTLE:
    std::copy(arr.pointer(), arr.pointer() + size, base());
    break;

  case platform::Endian::BIG:
    for (std::size_t i{0}, rev_i{size - 1}; i < size; ++i, --rev_i)
    {
      base()[i] = arr[rev_i];
    }
    break;
  }
}

template <uint16_t S>  // NOLINT
template <typename T>
constexpr meta::IfHasNoIndex<T, UInt<S>> &UInt<S>::operator=(T const &v)  // NOLINT
{
  wide_.fill(0);
  wide_[0] = static_cast<WideType>(v);

  return *this;
}

/////////////////////////////////////////////
/// comparison operators for UInt objects ///
/////////////////////////////////////////////

// Suppresion of GCC-7 false-positive warning '-Werror=stringop-overflow=':
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83239
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wstringop-overflow=0"
#endif  //__GNUC__

template <uint16_t S>
constexpr bool UInt<S>::operator==(UInt const &other) const
{
  std::size_t i = 0ull;
  for (; i < WIDE_ELEMENTS - 1; ++i)
  {
    if (wide_[i] != other.ElementAt(i))
    {
      return false;
    }
  }
  return (wide_[i] & RESIDUAL_BITS_MASK) == (other.ElementAt(i) & other.RESIDUAL_BITS_MASK);
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

    return wide_[WIDE_ELEMENTS - 1 - i] < other.ElementAt(WIDE_ELEMENTS - 1 - i);
  }
  return false;
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif  //__GNUC__

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
  return (*this == UInt(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator!=(T other) const
{
  return (*this != UInt(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator<(T other) const
{
  return (*this < UInt(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator>(T other) const
{
  return (*this > UInt(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator<=(T other) const
{
  return (*this <= UInt(other));
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, bool> UInt<S>::operator>=(T other) const
{
  return (*this >= UInt(other));
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

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator+() const
{
  return *this;
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator~() const
{
  UInt retval;
  for (std::size_t i{0}; i < WIDE_ELEMENTS; ++i)
  {
    retval.wide_[i] = ~wide_[i];
  }

  return retval;
}

//////////////////////////////
/// math and bit operators ///
//////////////////////////////

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator+(UInt const &n) const
{
  UInt ret{*this};
  ret += n;

  return ret;
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator-(UInt const &n) const
{
  UInt ret{*this};
  ret -= n;

  return ret;
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator*(UInt const &n) const
{
  UInt ret{*this};
  ret *= n;

  return ret;
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator/(UInt const &n) const
{
  UInt ret{*this};
  ret /= n;

  return ret;
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator%(UInt const &n) const
{
  UInt ret{*this};
  ret %= n;

  return ret;
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator&(UInt const &n) const
{
  UInt ret{*this};
  ret &= n;

  return ret;
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator|(UInt const &n) const
{
  UInt ret{*this};
  ret |= n;

  return ret;
}

template <uint16_t S>
constexpr UInt<S> UInt<S>::operator^(UInt const &n) const
{
  UInt ret{*this};
  ret ^= n;

  return ret;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator+=(UInt const &n)
{
  UInt::WideType carry = 0, new_carry = 0;
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
constexpr UInt<S> &UInt<S>::operator-=(UInt const &n)
{
  if (*this < n)
  {
    *this = _0;
  }
  UInt::WideType carry = 0, new_carry = 0;
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
  // No fractions supported, if *this < n return 0
  if (*this < n)
  {
    *this = _0;
    return *this;
  }

  // Actual integer division algorithm
  // First simplify dividend/divisor by shifting right by min LSB bit
  // Essentially divide both by 2 as long as the LSB is zero
  UInt        N{*this}, D{n};
  std::size_t lsb = std::min(N.lsb(), D.lsb());
  N >>= lsb;
  D >>= lsb;
  UInt multiple(1u);

  auto const D_leading_zero_bits{D.UINT_SIZE - D.msb() - 1};
  // Find smallest multiple of divisor (D) that is larger than the dividend (N)
  D <<= D_leading_zero_bits;
  multiple <<= D_leading_zero_bits;

  // Calculate Quotient in a loop, essentially divide divisor by 2 and subtract from Remainder
  // Add multiple to Quotient
  UInt Q = _0, R = N;
  do
  {
    if (R >= D)
    {
      R -= D;
      Q += multiple;
    }
    D >>= 1;  // Divide by two.
    multiple >>= 1;
  } while (multiple != UInt::_0);
  // Return the Quotient
  *this = Q;

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator%=(UInt const &n)
{
  UInt q = *this / n;
  *this -= q * n;
  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator&=(UInt const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    wide_[i] &= n.ElementAt(i);
  }

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator|=(UInt const &n)
{
  for (std::size_t i = 0; i < WIDE_ELEMENTS; ++i)
  {
    wide_[i] |= n.ElementAt(i);
  }

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator^=(UInt const &n)
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
  UInt ret{*this}, nint{n};
  ret += nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator-(T n) const
{
  UInt ret{*this}, nint{n};
  ret -= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator*(T n) const
{
  UInt ret{*this}, nint{n};
  ret *= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator/(T n) const
{
  UInt ret{*this}, nint{n};
  ret /= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator%(T n) const
{
  UInt ret{*this}, nint{n};
  ret %= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator&(T n) const
{
  UInt ret{*this}, nint{n};
  ret &= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator|(T n) const
{
  UInt ret{*this}, nint{n};
  ret |= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> UInt<S>::operator^(T n) const
{
  UInt ret{*this}, nint{n};
  ret ^= nint;

  return ret;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator+=(T n)
{
  UInt nint{n};
  *this += nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator-=(T n)
{
  UInt nint{n};
  *this -= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator*=(T n)
{
  UInt nint{n};
  *this *= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator/=(T n)
{
  UInt nint{n};
  *this /= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator%=(T n)
{
  UInt nint{n};
  *this %= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator&=(T n)
{
  UInt nint{n};
  *this &= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator|=(T n)
{
  UInt nint{n};
  *this |= nint;

  return *this;
}

template <uint16_t S>
template <typename T>
constexpr meta::IfIsUnsignedInteger<T, UInt<S>> &UInt<S>::operator^=(T n)
{
  UInt nint{n};
  *this ^= nint;

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator<<=(std::size_t bits)
{
  std::size_t full_words = bits / WIDE_ELEMENT_SIZE;
  std::size_t real_bits  = bits - full_words * WIDE_ELEMENT_SIZE;
  std::size_t nbits      = WIDE_ELEMENT_SIZE - real_bits;

  // No actual shifting involved, just move the elements
  if (full_words != 0u)
  {
    for (std::size_t i = WIDE_ELEMENTS - 1; i >= full_words; --i)
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

  mask_residual_bits();

  return *this;
}

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator>>=(std::size_t bits)
{
  std::size_t full_words = bits / WIDE_ELEMENT_SIZE;
  std::size_t real_bits  = bits - full_words * WIDE_ELEMENT_SIZE;
  std::size_t nbits      = WIDE_ELEMENT_SIZE - real_bits;

  mask_residual_bits();

  // No actual shifting involved, just move the elements
  if (full_words != 0u)
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

  return *this;
}

template <uint16_t S>
constexpr std::size_t UInt<S>::msb() const
{
  auto const trimmed_size{TrimmedWideSize()};
  if (trimmed_size == 0)
  {
    return UINT_SIZE;
  }

  auto const trimmed_idx{trimmed_size - 1};
  auto const last_elem_val{(trimmed_size == WIDE_ELEMENTS) ? wide_[trimmed_idx] & RESIDUAL_BITS_MASK
                                                           : wide_[trimmed_idx]};
  auto const leading_zero_bits = platform::CountLeadingZeroes64(last_elem_val);
  return trimmed_size * WIDE_ELEMENT_SIZE - leading_zero_bits - 1;
}

template <uint16_t S>
constexpr std::size_t UInt<S>::lsb() const
{
  std::size_t lsb = 0;
  std::size_t i   = 0;

  for (; i < WIDE_ELEMENTS - 1; i++, lsb += WIDE_ELEMENT_SIZE)
  {
    auto const val{wide_[i]};
    if (val > 0)
    {
      return lsb + platform::CountTrailingZeroes64(val);
    }
  }

  auto const val{wide_[i] & RESIDUAL_BITS_MASK};
  return val > 0 ? lsb + platform::CountTrailingZeroes64(val) : static_cast<uint64_t>(UINT_SIZE);
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
constexpr uint64_t UInt<S>::TrimmedWideSize() const
{
  uint64_t ret = WIDE_ELEMENTS;
  if ((wide_[ret - 1] & RESIDUAL_BITS_MASK) > 0)
  {
    return ret;
  }

  --ret;
  while ((ret > 0) && (wide_[ret - 1] == 0))
  {
    --ret;
  }
  return ret;
}

template <uint16_t S>
constexpr uint64_t UInt<S>::TrimmedSize() const
{
  uint64_t wide_size{TrimmedWideSize()};

  if (wide_size == 0)
  {
    return 0;
  }

  uint64_t remainder{wide_size == WIDE_ELEMENTS
                         ? (WIDE_ELEMENT_SIZE - RESIDUAL_BITS_MASK) / ELEMENT_SIZE
                         : WIDE_ELEMENT_SIZE / ELEMENT_SIZE};
  uint64_t start_idx{(wide_size - 1) * (WIDE_ELEMENT_SIZE / ELEMENT_SIZE)};

  while ((remainder > 0) && (base()[start_idx + remainder - 1] == 0))
  {
    --remainder;
  }

  return start_idx + remainder;
}

template <uint16_t S>
constexpr uint8_t const *UInt<S>::pointer() const
{
  return reinterpret_cast<uint8_t const *>(wide_.data());
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

template <uint16_t S>
template <typename T>
constexpr meta::EnableIf<std::is_same<meta::Decay<T>, byte_array::ByteArray>::value, T> UInt<S>::As(
    platform::Endian endianess_of_output_data, bool include_leading_zeroes) const
{
  auto const size_{include_leading_zeroes ? size() : TrimmedSize()};

  T arr;

  if (0 == size_)
  {
    return arr;
  }

  switch (endianess_of_output_data)
  {
  case platform::Endian::LITTLE:
    return T{pointer(), size_};

  case platform::Endian::BIG:
    arr.Resize(size_);
    for (std::size_t i{0}, rev_i{size_ - 1}; i < size_; ++i, --rev_i)
    {
      arr[rev_i] = base()[i];
    }
    return arr;
  }

  return arr;  // make gcc happy
}

template <uint16_t S>
std::ostream &operator<<(std::ostream &s, UInt<S> const &x)
{
  s << std::string(x);
  return s;
}

}  // namespace vectorise
}  // namespace fetch
