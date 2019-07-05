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
#include <cstdint>
#include <algorithm>
#include <array>
#include <cmath>
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
    UINT_SIZE = S,
    ELEMENT_SIZE = sizeof(BaseType) * 8,
    ELEMENTS  = UINT_SIZE / ELEMENT_SIZE,
    WIDE_ELEMENT_SIZE = sizeof(WideType) * 8,
    WIDE_ELEMENTS  = UINT_SIZE / WIDE_ELEMENT_SIZE
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
  constexpr UInt  operator&(UInt const &n) const;
  constexpr UInt  operator|(UInt const &n) const;
  constexpr UInt  operator^(UInt const &n) const;
  constexpr UInt  &operator+=(UInt const &n) const;
  constexpr UInt  &operator-=(UInt const &n) const;
  constexpr UInt  &operator*=(UInt const &n) const;
  constexpr UInt  &operator/=(UInt const &n) const;
  constexpr UInt  &operator&=(UInt const &n) const;
  constexpr UInt  &operator|=(UInt const &n) const;
  constexpr UInt  &operator^=(UInt const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator+(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator-(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator*(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt> operator/(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>operator&(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>operator|(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>operator^(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>&operator+=(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>&operator-=(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>&operator&=(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>&operator|=(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>&operator^=(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>&operator*=(T const &n) const;
  template <typename T>
  constexpr meta::IfIsInteger<T, UInt>&operator/=(T const &n) const;

  constexpr UInt &operator<<=(std::size_t const &n);
  constexpr UInt &operator>>=(std::size_t const &n);

  /////////////////////////
  /// element accessors ///
  /////////////////////////

  constexpr uint8_t operator[](std::size_t const &n) const;
  constexpr uint8_t &operator[](std::size_t const &n);
  constexpr WideType elementAt(std::size_t const &n) const;
  constexpr WideType &elementAt(std::size_t const &n);

  constexpr uint64_t TrimmedSize() const;
  constexpr uint64_t size() const;

  constexpr uint8_t const *pointer() const;

  template <typename T, uint16_t G>
  constexpr friend void Serialize(T &s, UInt<G> const &u);

  template <typename T, uint16_t G>
  constexpr friend void Deserialize(T &s, UInt<G> &u);

  explicit operator std::string() const;

private:
  union
  {
    ContainerType base;
    WideContainerType wide;
  } data_;
};

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
  for (std::size_t i = WIDE_ELEMENTS -1; i >= 0; --i)
  {
    if (data_.wide[i] == other.elementAt(i))
    {
      continue;
    }
    else {
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
  for (std::size_t i = 0; i < data_.wide.size(); ++i)
  {
    ++data_.wide[i];
    // If element is not zero, then we don't have an overflow and nothing to carry, stop here
    if (data_.wide[i]) {
      break;
    }
    if (i == data_.wide.size())
    {
      throw std::runtime_error("Overflow error in UInt object, use a larger size");
    }
  }

  return *this;
}

//////////////////////////////
/// math and bit operators ///
//////////////////////////////

template <uint16_t S>
constexpr UInt<S> &UInt<S>::operator<<=(std::size_t const &bits)
{
  assert(bits < WIDE_ELEMENT_SIZE);
  std::size_t nbits = WIDE_ELEMENT_SIZE - bits;
  WideType carry = 0;
  for (std::size_t i = 0; i < data_.wide.size(); ++i)
  {
    std::cout << "*this         = " << std::string(*this) << std::endl;
    std::cout << "bits          = " << std::dec << bits << std::endl;
    std::cout << "nbits         = " << std::dec << nbits << std::endl;
    std::cout << "carry (old)   = " << std::hex << carry << std::endl;
    WideType val = data_.wide[i];
    std::cout << "data_.wide[" << i << "] = " << std::hex << val << std::endl;
    std::cout << "val = " << std::hex << (val << bits) << std::endl;

    data_.wide[i] = (val << bits) | carry;
    std::cout << "data_.wide[" << i << "] = " << std::hex << data_.wide[i] << std::endl;
    std::cout << "*this = " << std::string(*this) << std::endl;
    carry         = val >> nbits;
    std::cout << "carry (new)    = " << std::hex << carry << std::endl;
  }

  return *this;
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
  uint64_t ret = data_.wide.size();
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
  return data_.wide.size();
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
