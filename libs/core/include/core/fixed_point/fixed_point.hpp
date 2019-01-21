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

#include "fixed_point_tag.hpp"
#include <iostream>
#include <sstream>

//#include <ostream>
//#include <exception>
//#include <cstddef>      // std::size_t
//#include <cstdint>
//#include <type_traits>

#include "meta/type_traits.hpp"
#include <cassert>
#include <limits>

namespace fetch {
namespace fixed_point {

template <std::size_t I, std::size_t F>
class FixedPoint;

namespace details {

// struct for inferring what underlying types to use
template <std::size_t T>
struct TypeFromSize
{
  static const bool is_valid = false;  // for template matches specialisation
  using ValueType            = void;
};

// 64 bit implementation
template <>
struct TypeFromSize<64>
{
  static const bool        is_valid = true;
  static const std::size_t size     = 64;
  using ValueType                   = int64_t;
  using UnsignedType                = uint64_t;
  using SignedType                  = int64_t;
  using NextSize                    = TypeFromSize<128>;
};

// 32 bit implementation
template <>
struct TypeFromSize<32>
{
  static const bool        is_valid = true;
  static const std::size_t size     = 32;
  using ValueType                   = int32_t;
  using UnsignedType                = uint32_t;
  using SignedType                  = int32_t;
  using NextSize                    = TypeFromSize<64>;
};

// 16 bit implementation
template <>
struct TypeFromSize<16>
{
  static const bool        is_valid = true;
  static const std::size_t size     = 16;
  using ValueType                   = int16_t;
  using UnsignedType                = uint16_t;
  using SignedType                  = int16_t;
  using NextSize                    = TypeFromSize<32>;
};

// 8 bit implementation
template <>
struct TypeFromSize<8>
{
  static const bool        is_valid = true;
  static const std::size_t size     = 8;
  using ValueType                   = int8_t;
  using UnsignedType                = uint8_t;
  using SignedType                  = int8_t;
  using NextSize                    = TypeFromSize<16>;
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
template <std::size_t I, std::size_t F>
FixedPoint<I, F> Divide(const FixedPoint<I, F> & /*numerator*/,
                        const FixedPoint<I, F> & /*denominator*/, FixedPoint<I, F> & /*remainder*/)
{
  assert(0);  // not yet implemented
  FixedPoint<I, F> quotient(0);
  return quotient;
}

/**
 * multiplies two fixed points together
 * @tparam I
 * @tparam F
 * @param lhs
 * @param rhs
 * @param result
 */
template <std::size_t I, std::size_t F>
void Multiply(const FixedPoint<I, F> & /*lhs*/, const FixedPoint<I, F> & /*rhs*/,
              FixedPoint<I, F> & /*result*/)
{
  assert(0);  // not yet implemented
}

/**
 * finds most significant set bit in type
 * @tparam T
 * @param n
 * @return
 */
template <typename T>
std::size_t HighestSetBit(T n_input)
{
  int n = static_cast<int>(n_input);

  if (n == 0)
  {
    return 0;
  }

  std::size_t msb = 0;
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
 * @param fractional_bits the number of fractional_bits to be set
 * @param total_bits the total number of bits
 * @return true if there is no overflow, false otherwise
 */
template <typename T>
bool CheckNoOverflow(T n, std::size_t fractional_bits, std::size_t total_bits)
{
  std::size_t hsb = HighestSetBit(n);
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
bool CheckNoRounding(T n, std::size_t fractional_bits)
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

}  // namespace details

template <std::size_t I, std::size_t F>
class FixedPoint
{
  static_assert(details::TypeFromSize<I + F>::is_valid, "invalid combination of sizes");

public:
  static const std::size_t fractional_bits = F;
  static const std::size_t total_bits      = I + F;

  // this tag is used for matching templates - see math_type_traits.hpp
  FixedPointTag fixed_point_tag;

  using BaseTypeInfo = details::TypeFromSize<total_bits>;

  using Type         = typename BaseTypeInfo::ValueType;
  using NextType     = typename BaseTypeInfo::NextSize::ValueType;
  using UnsignedType = typename BaseTypeInfo::UnsignedType;

  const Type fractional_mask = UnsignedType((2 ^ fractional_bits) - 1);
  const Type integer_mask    = ~fractional_mask;

  static const Type one = Type(1) << fractional_bits;

private:
  Type data_;  // the value to be stored

public:
  ////////////////////
  /// constructors ///
  ////////////////////

  FixedPoint()
    : data_(0)
  {}  // initialise to zero

  template <typename T>
  explicit FixedPoint(T n, meta::IfIsInteger<T> * = nullptr)
    : data_(static_cast<Type>(n) << static_cast<Type>(fractional_bits))
  {
    assert(details::CheckNoOverflow(n, fractional_bits, total_bits));
  }

  template <typename T>
  explicit FixedPoint(T n, meta::IfIsFloat<T> * = nullptr)
    : data_(static_cast<Type>(n * one))
  {
    assert(details::CheckNoOverflow(n, fractional_bits, total_bits));
    assert(details::CheckNoRounding(n, fractional_bits));
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

  bool operator==(const FixedPoint &o) const
  {
    return data_ == o.data_;
  }

  bool operator<(const FixedPoint &o) const
  {
    return data_ < o.data_;
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
    data_ += one;
    return *this;
  }

  FixedPoint &operator--()
  {
    data_ -= one;
    return *this;
  }

  //  operator sizeof() const
  //  {
  //    return total_bits;
  //  }

  /////////////////////////
  /// casting operators ///
  /////////////////////////

  operator double() const
  {
    return (static_cast<double>(data_) / one);
  }

  operator int() const
  {
    return (data_ & integer_mask) >> fractional_bits;
  }

  operator float() const
  {
    return (static_cast<float>(data_) / one);
  }

  //  // casting operators
  //  operator uint32_t () const {
  //    return (data_ & integer_mask) >> fractional_bits;
  //  }

  //////////////////////
  /// math operators ///
  //////////////////////

  FixedPoint operator+(const FixedPoint &n) const
  {
    return FixedPoint(data_ + n.data_);
  }

  template <typename T>
  FixedPoint operator+(const T &n) const
  {
    return FixedPoint(T(data_) + n);
  }

  FixedPoint operator-(const FixedPoint &n) const
  {
    return FixedPoint(data_ - n.data_);
  }

  template <typename T>
  FixedPoint operator-(const T &n) const
  {
    return FixedPoint(T(data_) - n);
  }

  FixedPoint operator*(const FixedPoint &n) const
  {
    return FixedPoint(data_ * n.data_);
  }

  template <typename T>
  FixedPoint operator*(const T &n) const
  {
    return FixedPoint(T(data_) * n);
  }

  FixedPoint operator/(const FixedPoint &n) const
  {
    return FixedPoint(data_ / n.data_);
  }

  template <typename T>
  FixedPoint operator/(const T &n) const
  {
    return FixedPoint(T(data_) / n);
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
    details::Multiply(*this, n, *this);
    return *this;
  }

  FixedPoint &operator/=(const FixedPoint &n)
  {
    FixedPoint temp;
    *this = details::Divide(*this, n, temp);
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

private:
  // this makes it simpler to create a fixed point object from
  // a native type without scaling
  // use "FixedPoint::from_base" in order to perform this.
  struct NoScale
  {
  };

  FixedPoint(Type n, const NoScale &)
    : data_(n)
  {}

public:
  static FixedPoint FromBase(Type n)
  {
    return FixedPoint(n, NoScale());
  }
};

}  // namespace fixed_point
}  // namespace fetch
