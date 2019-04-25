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

#include "meta/tags.hpp"
#include "meta/type_traits.hpp"

#include <iostream>
#include <sstream>

#include <cassert>
#include <iomanip>
#include <limits>

namespace fetch {
namespace fixed_point {

template <std::uint16_t I, std::uint16_t F>
class FixedPoint;

namespace {

// struct for inferring what underlying types to use
template <int T>
struct TypeFromSize
{
  static const bool is_valid = false;  // for template matches specialisation
  using ValueType            = void;
};

// 64 bit implementation
template <>
struct TypeFromSize<64>
{
  static const bool          is_valid = true;
  static const std::uint16_t size     = 64;
  using ValueType                     = int64_t;
  using UnsignedType                  = uint64_t;
  using SignedType                    = int64_t;
  using NextSize                      = TypeFromSize<128>;
};

// 32 bit implementation
template <>
struct TypeFromSize<32>
{
  static const bool          is_valid = true;
  static const std::uint16_t size     = 32;
  using ValueType                     = int32_t;
  using UnsignedType                  = uint32_t;
  using SignedType                    = int32_t;
  using NextSize                      = TypeFromSize<64>;
};

// 16 bit implementation
template <>
struct TypeFromSize<16>
{
  static const bool          is_valid = true;
  static const std::uint16_t size     = 16;
  using ValueType                     = int16_t;
  using UnsignedType                  = uint16_t;
  using SignedType                    = int16_t;
  using NextSize                      = TypeFromSize<32>;
};

// 8 bit implementation
template <>
struct TypeFromSize<8>
{
  static const bool          is_valid = true;
  static const std::uint16_t size     = 8;
  using ValueType                     = int8_t;
  using UnsignedType                  = uint8_t;
  using SignedType                    = int8_t;
  using NextSize                      = TypeFromSize<16>;
};

/**
 * Divides two fixed points
 * @tparam I
 * @tparam F
 * @param numerator
 * @param denominator
 * @param remainder
 * @return
 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> Divide(const FixedPoint<I, F> &numerator, const FixedPoint<I, F> &denominator,
                        FixedPoint<I, F> & /*remainder*/)
{
  // TODO(private, 501) --
  return numerator / denominator;
}

/**
 * multiplies two fixed points together
 * @tparam I
 * @tparam F
 * @param lhs
 * @param rhs
 * @param result
 */
template <std::uint16_t I, std::uint16_t F>
void Multiply(const FixedPoint<I, F> &lhs, const FixedPoint<I, F> &rhs, FixedPoint<I, F> &result)
{
  // TODO(private, 501) -- Remove cast
  result = rhs * lhs;
}

/**
 * finds most significant set bit in type
 * @tparam T
 * @param n
 * @return
 */
template <typename T>
std::uint16_t HighestSetBit(T n_input)
{
  int n = static_cast<int>(n_input);

  if (n == 0)
  {
    return 0;
  }

  std::uint16_t msb = 0;
  while (n != 0)
  {
    n = n / 2;
    msb++;
  }

  return msb;
}

/**
 * helper function that checks no bit overflow when shifting
 * @tparam T the input original type
 * @param n the value of the datum
 * @param FRACTIONAL_BITS the number of FRACTIONAL_BITS to be set
 * @param total_bits the total number of bits
 * @return true if there is no overflow, false otherwise
 */
template <typename T>
bool CheckNoOverflow(T n, std::uint16_t fractional_bits, std::uint16_t total_bits)
{
  std::uint16_t hsb = HighestSetBit(n);
  if (hsb + fractional_bits <= total_bits)
  {
    return true;
  }
  std::cout << "OVERFLOW ERROR! " << std::endl;
  std::cout << "n: " << n << std::endl;
  std::cout << "HighestSetBit(n): " << hsb << std::endl;
  std::cout << "fractional_bits: " << fractional_bits << std::endl;
  std::cout << "hsb + fractional_bits: " << hsb + fractional_bits << std::endl;
  std::cout << "total_bits: " << total_bits << std::endl;
  return false;
}

/**
 * helper function that checks no rounding error when casting
 * @tparam T
 * @param n
 * @param fractional_bits
 * @param total_bits
 * @return
 */
template <typename T>
bool CheckNoRounding(T n, std::uint16_t fractional_bits)
{
  T original_n = n;

  // sufficient bits to guarantee no rounding
  if (std::numeric_limits<T>::max_digits10 < fractional_bits)
  {
    return true;
  }

  std::cout << "ROUNDING ERROR! " << std::endl;
  std::cout << "n: " << original_n << std::endl;
  std::cout << "fractional_bits: " << fractional_bits << std::endl;
  return false;
}

}  // namespace

struct BaseFixedpointType
{
};

template <std::uint16_t I, std::uint16_t F>
class FixedPoint : public BaseFixedpointType
{
  static_assert(TypeFromSize<I + F>::is_valid, "invalid combination of sizes");

public:
  enum
  {
    FRACTIONAL_BITS = F,
    TOTAL_BITS      = I + F
  };

  using BaseTypeInfo = TypeFromSize<TOTAL_BITS>;
  using Type         = typename BaseTypeInfo::ValueType;
  using NextType     = typename BaseTypeInfo::NextSize::ValueType;
  using UnsignedType = typename BaseTypeInfo::UnsignedType;

  static const FixedPoint CONST_SMALLEST_FRACTION;
  static const Type       SMALLEST_FRACTION;
  static const Type       LARGEST_FRACTION;
  static const Type       MAX_INT;
  static const Type       MIN_INT;
  static const Type       MAX;
  static const Type       MIN;

  enum
  {
    FRACTIONAL_MASK = Type(((1ull << FRACTIONAL_BITS) - 1)),
    INTEGER_MASK    = Type(~FRACTIONAL_MASK),
    CONST_ONE       = Type(1) << FRACTIONAL_BITS
  };

  ////////////////////
  /// constructors ///
  ////////////////////
  FixedPoint() = default;

  /**
   * Templated constructor existing only for T is an integer and assigns data
   * @tparam T any integer type
   * @param n integer value to set FixedPoint to
   */
  template <typename T>
  explicit FixedPoint(T n, meta::IfIsInteger<T> * = nullptr)
    : data_(static_cast<Type>(n) << static_cast<Type>(FRACTIONAL_BITS))
  {
    assert(CheckNoOverflow(n, FRACTIONAL_BITS, TOTAL_BITS));
  }

  template <typename T>
  explicit FixedPoint(T n, meta::IfIsFloat<T> * = nullptr)
    : data_(static_cast<Type>(n * CONST_ONE))
  {
    assert(CheckNoOverflow(n, FRACTIONAL_BITS, TOTAL_BITS));
    // TODO(private, 629)
    // assert(CheckNoRounding(n, FRACTIONAL_BITS));
  }

  FixedPoint(const FixedPoint &o)
    : data_(o.data_)
  {}

  /////////////////
  /// operators ///
  /////////////////

  FixedPoint &operator=(const FixedPoint &o)
  {
    data_ = o.data_;
    return *this;
  }

  ////////////////////////////
  /// comparison operators ///
  ////////////////////////////

  template <typename OtherType>
  bool operator==(const OtherType &o) const
  {
    return (data_ == FixedPoint(o).data_);
  }

  template <typename OtherType>
  bool operator!=(const OtherType &o) const
  {
    return data_ != FixedPoint(o).data_;
  }

  template <typename OtherType>
  constexpr bool operator<(const OtherType &o) const
  {
    return data_ < FixedPoint(o).data_;
  }

  template <typename OtherType>
  constexpr bool operator>(const OtherType &o) const
  {
    return data_ > FixedPoint(o).data_;
  }

  template <typename OtherType>
  bool operator<=(const OtherType &o) const
  {
    return data_ <= FixedPoint(o).data_;
  }

  template <typename OtherType>
  bool operator>=(const OtherType &o) const
  {
    return data_ >= FixedPoint(o).data_;
  }

  ///////////////////////
  /// unary operators ///
  ///////////////////////

  bool operator!() const
  {
    return !data_;
  }

  FixedPoint operator~() const
  {
    FixedPoint t(*this);
    t.data_ = ~t.data_;
    return t;
  }

  FixedPoint operator-() const
  {
    FixedPoint t(*this);
    t.data_ = -t.data_;
    return t;
  }

  FixedPoint &operator++()
  {
    data_ += CONST_ONE;
    return *this;
  }

  FixedPoint &operator--()
  {
    data_ -= CONST_ONE;
    return *this;
  }

  //  operator sizeof() const
  //  {
  //    return total_bits;
  //  }

  /////////////////////////
  /// casting operators ///
  /////////////////////////

  explicit operator double() const
  {
    return (static_cast<double>(data_) / CONST_ONE);
  }

  explicit operator int() const
  {
    return int((data_ & INTEGER_MASK) >> FRACTIONAL_BITS);
  }

  explicit operator float() const
  {
    return (static_cast<float>(data_) / CONST_ONE);
  }

  explicit operator unsigned() const
  {
    return (static_cast<unsigned>(data_) / CONST_ONE);
  }

  explicit operator unsigned long() const
  {
    return (static_cast<unsigned long>(data_) / CONST_ONE);
  }

  explicit operator unsigned long long() const
  {
    return (static_cast<unsigned long long>(data_) / CONST_ONE);
  }

  //////////////////////
  /// math operators ///
  //////////////////////

  FixedPoint operator+(const FixedPoint &n) const
  {
    FixedPoint fp;
    fp.data_ = data_ + n.data_;
    return fp;
  }

  template <typename T>
  FixedPoint operator+(const T &n) const
  {
    return FixedPoint(T(data_) + n);
  }

  FixedPoint operator-(const FixedPoint &n) const
  {
    FixedPoint fp;
    fp.data_ = data_ - n.data_;
    return fp;
  }

  template <typename T>
  FixedPoint operator-(const T &n) const
  {
    return FixedPoint(T(data_) - n);
  }

  FixedPoint operator*(const FixedPoint &n) const
  {
    // TODO(private, 501)
    return FixedPoint(double(*this) * double(n));
  }

  template <typename T>
  FixedPoint operator*(const T &n) const
  {
    return FixedPoint(T(data_) * n);
  }

  FixedPoint operator/(const FixedPoint &n) const
  {
    // TODO(private, 501)
    return FixedPoint(double(*this) / double(n));
  }

  template <typename T>
  FixedPoint operator/(const T &n) const
  {
    return *this / FixedPoint(n);
  }

  FixedPoint &operator+=(const FixedPoint &n)
  {
    data_ += n.data_;
    return *this;
  }

  FixedPoint &operator-=(const FixedPoint &n)
  {
    data_ -= n.data_;
    return *this;
  }

  FixedPoint &operator&=(const FixedPoint &n)
  {
    data_ &= n.data_;
    return *this;
  }

  FixedPoint &operator|=(const FixedPoint &n)
  {
    data_ |= n.data_;
    return *this;
  }

  FixedPoint &operator^=(const FixedPoint &n)
  {
    data_ ^= n.data_;
    return *this;
  }

  FixedPoint &operator*=(const FixedPoint &n)
  {
    Multiply(*this, n, *this);
    return *this;
  }

  FixedPoint &operator/=(const FixedPoint &n)
  {
    FixedPoint temp;
    *this = Divide(*this, n, temp);
    return *this;
  }

  FixedPoint &operator>>=(const FixedPoint &n)
  {
    data_ >>= n.to_int();
    return *this;
  }

  FixedPoint &operator<<=(const FixedPoint &n)
  {
    data_ <<= n.to_int();
    return *this;
  }

  ////////////
  /// swap ///
  ////////////

  void Swap(FixedPoint &rhs)
  {
    std::swap(data_, rhs.data_);
  }

  Type Data() const
  {
    return data_;
  }

  static FixedPoint FromBase(Type n)
  {
    return FixedPoint(n, NoScale());
  }

private:
  Type data_{0};  // the value to be stored

  // this makes it simpler to create a fixed point object from
  // a native type without scaling
  // use "FixedPoint::from_base" in order to perform this.
  struct NoScale
  {
  };

  FixedPoint(Type n, const NoScale &)
    : data_(n)
  {}
};

template <std::uint16_t I, std::uint16_t F>
std::ostream &operator<<(std::ostream &s, FixedPoint<I, F> const &n)
{
  std::ios_base::fmtflags f(s.flags());
  s << std::setprecision(F);
  s << std::fixed;
  s << double(n);
  s.flags(f);
  return s;
}

template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_SMALLEST_FRACTION(
    0, FixedPoint::SMALLEST_FRACTION); /* 0 */
template <std::uint16_t I, std::uint16_t F>
const typename FixedPoint<I, F>::Type FixedPoint<I, F>::SMALLEST_FRACTION{
    1}; /* smallest fraction */
template <std::uint16_t I, std::uint16_t F>
const typename FixedPoint<I, F>::Type FixedPoint<I, F>::LARGEST_FRACTION{
    FRACTIONAL_MASK}; /* largest fraction */
template <std::uint16_t I, std::uint16_t F>
const typename FixedPoint<I, F>::Type FixedPoint<I, F>::MAX_INT =
    Type(FRACTIONAL_MASK >> 1) << FRACTIONAL_BITS; /* largest int */
template <std::uint16_t I, std::uint16_t F>
const typename FixedPoint<I, F>::Type FixedPoint<I, F>::MIN_INT =
    INTEGER_MASK &((Type(1) << (TOTAL_BITS - 1))); /* smallest int */
template <std::uint16_t I, std::uint16_t F>
const typename FixedPoint<I, F>::Type FixedPoint<I, F>::MAX =
    MAX_INT | LARGEST_FRACTION; /* largest fixed point */
template <std::uint16_t I, std::uint16_t F>
const typename FixedPoint<I, F>::Type FixedPoint<I, F>::MIN =
    MIN_INT ^ LARGEST_FRACTION; /* smallest fixed point */

}  // namespace fixed_point
}  // namespace fetch
