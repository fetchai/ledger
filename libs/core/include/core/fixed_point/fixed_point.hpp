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

#include <core/assert.hpp>
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

#if (__SIZEOF_INT128__ == 16)
// 128 bit implementation
template <>
struct TypeFromSize<128>
{
  static const bool        is_valid = true;
  static const std::size_t size     = 128;
  using ValueType                   = __int128_t;
  using UnsignedType                = __uint128_t;
  using SignedType                  = __int128_t;
  using NextSize                    = TypeFromSize<128>;
};
#endif

// 64 bit implementation
template <>
struct TypeFromSize<64>
{
  static const bool        is_valid  = true;
  static const std::size_t size      = 64;
  using ValueType                    = int64_t;
  using UnsignedType                 = uint64_t;
  using SignedType                   = int64_t;
  using NextSize                     = TypeFromSize<128>;
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
inline FixedPoint<I, F> Divide(const FixedPoint<I, F> &numerator, const FixedPoint<I, F> &denominator,
                        FixedPoint<I, F> & /*remainder*/)
{
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
inline void Multiply(const FixedPoint<I, F> &lhs, const FixedPoint<I, F> &rhs, FixedPoint<I, F> &result)
{
  result = rhs * lhs;
}

/**
 * finds most significant set bit in type
 * @tparam T
 * @param n
 * @return
 */
template <typename T>
inline std::uint16_t HighestSetBit(T n_input)
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
inline bool CheckNoOverflow(T n, std::uint16_t fractional_bits, std::uint16_t total_bits)
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
inline bool CheckNoRounding(T n, std::uint16_t fractional_bits)
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

  enum
  {
    FRACTIONAL_MASK = Type(((1ull << FRACTIONAL_BITS) - 1)),
    INTEGER_MASK    = Type(~FRACTIONAL_MASK),
    CONST_ONE       = Type(1) << FRACTIONAL_BITS
  };

  // Constants/Limits
  static const Type one = Type(1) << fractional_bits;
  const Type smallest_fraction = 1;
  const Type largest_fraction = fractional_mask;
  const Type largest_int = Type(fractional_mask >> 1) << fractional_bits;
  const Type smallest_int = integer_mask & ((Type(1) << (total_bits -1)));
  const Type max = largest_int | largest_fraction;
  const Type min = smallest_int - largest_fraction;

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

  inline FixedPoint(const FixedPoint &o)
    : data_(o.data_)
  {}

  inline FixedPoint(const Type &integer, const Type &fraction)
    : data_((integer_mask & (Type(integer) << fractional_bits)) | (fraction & fractional_mask))
  {}

  inline const Type integer() const 
  {
    return (data_ & integer_mask) >> fractional_bits;
  }

  inline const Type fraction() const
  {
    return (data_ & fractional_mask);
  }
  /////////////////
  /// operators ///
  /////////////////

  inline FixedPoint &operator=(const FixedPoint &o)
  {
    data_ = o.data_;
    return *this;
  }

  ////////////////////////////
  /// comparison operators ///
  ////////////////////////////

  template <typename OtherType>
  inline bool operator==(const OtherType &o) const
  {
    return (data_ == FixedPoint(o).data_);
  }

  template <typename OtherType>
  inline bool operator!=(const OtherType &o) const
  {
    return data_ != FixedPoint(o).data_;
  }

  template <typename OtherType>
  inline bool operator<(const OtherType &o) const
  {
    return data_ < FixedPoint(o).data_;
  }

  template <typename OtherType>
  inline bool operator>(const OtherType &o) const
  {
    return data_ > FixedPoint(o).data_;
  }

  template <typename OtherType>
  inline bool operator<=(const OtherType &o) const
  {
    return data_ <= FixedPoint(o).data_;
  }

  template <typename OtherType>
  inline bool operator>=(const OtherType &o) const
  {
    return data_ >= FixedPoint(o).data_;
  }

  ///////////////////////
  /// unary operators ///
  ///////////////////////

  inline bool operator!() const
  {
    return !data_;
  }

  inline FixedPoint operator~() const
  {
    FixedPoint t(*this);
    t.data_ = ~t.data_;
    return t;
  }

  inline FixedPoint operator-() const
  {
    FixedPoint t(*this);
    t.data_ = -t.data_;
    return t;
  }

  inline FixedPoint &operator++()
  {
    data_ += CONST_ONE;
    return *this;
  }

  inline FixedPoint &operator--()
  {
    data_ -= CONST_ONE;
    return *this;
  }

  /////////////////////////
  /// casting operators ///
  /////////////////////////

  inline explicit operator double() const
  {
    return (static_cast<double>(data_) / CONST_ONE);
  }

  inline explicit operator int() const
  {
    return int((data_ & INTEGER_MASK) >> FRACTIONAL_BITS);
  }

  inline explicit operator float() const
  {
    return (static_cast<float>(data_) / CONST_ONE);
  }

  inline explicit operator unsigned() const
  {
    return (static_cast<unsigned>(data_) / CONST_ONE);
  }

  inline explicit operator unsigned long() const
  {
    return (static_cast<unsigned long>(data_) / CONST_ONE);
  }

  inline explicit operator unsigned long long() const
  {
    return (static_cast<unsigned long long>(data_) / CONST_ONE);
  }

  //////////////////////
  /// math operators ///
  //////////////////////

  inline FixedPoint operator+(const FixedPoint &n) const
  {
    Type fp = data_ + n.Data();
    return FromBase(fp);
  }

  template <typename T>
  inline FixedPoint operator+(const T &n) const
  {
    return FixedPoint(T(data_) + n);
  }

  inline FixedPoint operator-(const FixedPoint &n) const
  {
    Type fp = data_ - n.Data();
    return FromBase(fp);
  }

  template <typename T>
  inline FixedPoint operator-(const T &n) const
  {
    return FixedPoint(T(data_) - n);
  }

  inline FixedPoint operator*(const FixedPoint &n) const
  {
    NextType prod = NextType(data_) * NextType(n.Data());
    Type fp = Type(prod >> fractional_bits);
    return FromBase(fp);
  }

  template <typename T>
  inline FixedPoint operator*(const T &n) const
  {
    return *this * FixedPoint(n);
  }

  inline FixedPoint operator/(const FixedPoint &n) const
  {
    if (n == 0) throw std::overflow_error("Divide by zero!");
    NextType numerator = NextType(data_) << fractional_bits;
    NextType quotient = numerator / NextType(n.Data());
    return FromBase(Type(quotient));
  }

  template <typename T>
  inline FixedPoint operator/(const T &n) const
  {
    return *this / FixedPoint(n);
  }

  inline FixedPoint &operator+=(const FixedPoint &n)
  {
    data_ += n.Data();
    return *this;
  }

  inline FixedPoint &operator-=(const FixedPoint &n)
  {
    data_ -= n.Data();
    return *this;
  }

  inline FixedPoint &operator&=(const FixedPoint &n)
  {
    data_ &= n.Data();
    return *this;
  }

  inline FixedPoint &operator|=(const FixedPoint &n)
  {
    data_ |= n.Data();
    return *this;
  }

  inline FixedPoint &operator^=(const FixedPoint &n)
  {
    data_ ^= n.Data();
    return *this;
  }

  inline FixedPoint &operator*=(const FixedPoint &n)
  {
    Multiply(*this, n, *this);
    return *this;
  }

  inline FixedPoint &operator/=(const FixedPoint &n)
  {
    FixedPoint temp;
    *this = Divide(*this, n, temp);
    return *this;
  }

  inline FixedPoint &operator>>=(const FixedPoint &n)
  {
    data_ >>= n.to_int();
    return *this;
  }

  inline FixedPoint &operator<<=(const FixedPoint &n)
  {
    data_ <<= n.to_int();
    return *this;
  }

  ////////////
  /// swap ///
  ////////////

  inline void Swap(FixedPoint &rhs)
  {
    std::swap(data_, rhs.Data());
  }

  inline Type Data() const
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

  inline FixedPoint(Type n, const NoScale &)
    : data_(n)
  {}
};

template <std::uint16_t I, std::uint16_t F>
std::ostream &operator<<(std::ostream &s, FixedPoint<I, F> const &n)
{
  std::ios_base::fmtflags f(s.flags());
  s << std::dec;
  s << n.integer();
  s << '.';
  s << n.fraction();
  s.flags(f);
  return s;
}

}  // namespace fixed_point
}  // namespace fetch
