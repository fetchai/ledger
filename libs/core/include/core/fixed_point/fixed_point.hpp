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
inline FixedPoint<I, F> Divide(const FixedPoint<I, F> &numerator, const FixedPoint<I, F> &denominator,
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
template <std::size_t I, std::size_t F>
inline void Multiply(const FixedPoint<I, F> &lhs, const FixedPoint<I, F> &rhs, FixedPoint<I, F> &result)
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
inline std::size_t HighestSetBit(T n_input)
{
  uint64_t n = n_input;

  if (n == 0)
  {
    return 0;
  }

  return sizeof(n)*8 - __builtin_clzll(n);
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
inline bool CheckNoOverflow(T n, std::size_t fractional_bits, std::size_t total_bits)
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
inline bool CheckNoRounding(T n, std::size_t fractional_bits)
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

  static const Type fractional_mask = Type((Type(1) << fractional_bits) - 1);
  static const Type integer_mask    = ~fractional_mask;

  // Constants/Limits
  static const Type one = Type(1) << fractional_bits;
  static const FixedPoint<I,F> One; /* e */
  static const FixedPoint<I,F> E; /* e */
  static const FixedPoint<I,F> LOG2E; /* log_2 e */
  static const FixedPoint<I,F> LOG10E; /* log_10 e */
  static const FixedPoint<I,F> LN2; /* log_e 2 */
  static const FixedPoint<I,F> LN10; /* log_e 10 */
  static const FixedPoint<I,F> PI; /* pi */
  static const FixedPoint<I,F> PI_2; /* pi/2 */
  static const FixedPoint<I,F> PI_4; /* pi/4 */
  static const FixedPoint<I,F> INV_PI; /* 1/pi */
  static const FixedPoint<I,F> INV2_PI; /* 2/pi */
  static const FixedPoint<I,F> INV2_SQRTPI; /* 2/sqrt(pi) */
  static const FixedPoint<I,F> SQRT2  ; /* sqrt(2) */
  static const FixedPoint<I,F> INV_SQRT2; /* 1/sqrt(2) */
  const Type smallest_fraction = 1;
  const Type largest_fraction = fractional_mask;
  const Type largest_int = Type(fractional_mask >> 1) << fractional_bits;
  const Type smallest_int = integer_mask & ((Type(1) << (total_bits -1)));
  const Type max = largest_int | largest_fraction;
  const Type min = smallest_int - largest_fraction;

  ////////////////////
  /// constructors ///
  ////////////////////

  inline FixedPoint()
    : data_{0}
  {}  // initialise to zero

  template <typename T>
  inline explicit FixedPoint(T n, meta::IfIsInteger<T> * = nullptr)
    : data_{static_cast<Type>(n) << static_cast<Type>(fractional_bits)}
  {
    assert(details::CheckNoOverflow(n, fractional_bits, total_bits));
  }

  template <typename T>
  inline explicit FixedPoint(T n, meta::IfIsFloat<T> * = nullptr)
    : data_{static_cast<Type>(n * one)}
  {
    assert(details::CheckNoOverflow(n, fractional_bits, total_bits));
    // TODO(private, 629)
    assert(details::CheckNoRounding(n, fractional_bits));
  }

  inline FixedPoint(const FixedPoint &o)
    : data_{o.data_}
  {}

  inline FixedPoint(const Type &integer, const Type &fraction)
    : data_{(integer_mask & (Type(integer) << fractional_bits)) | (fraction & fractional_mask)}
  {}

  inline const Type integer() const 
  {
    return Type((data_ & integer_mask) >> fractional_bits);
  }

  inline const Type fraction() const
  {
    return (data_ & fractional_mask);
  }

  inline const FixedPoint floor() const 
  {
    return FixedPoint(Type((data_ & integer_mask) >> fractional_bits));
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
    data_ += one;
    return *this;
  }

  inline FixedPoint &operator--()
  {
    data_ -= one;
    return *this;
  }

  /////////////////////////
  /// casting operators ///
  /////////////////////////

  inline explicit operator double() const
  {
    return (static_cast<double>(data_) / one);
  }

  inline explicit operator int() const
  {
    return int((data_ & integer_mask) >> fractional_bits);
  }

  inline explicit operator float() const
  {
    return (static_cast<float>(data_) / one);
  }

  inline explicit operator unsigned() const
  {
    return (static_cast<unsigned>(int()));
  }

  inline explicit operator unsigned long() const
  {
    return (static_cast<unsigned long>(int()));
  }

  inline explicit operator unsigned long long() const
  {
    return (static_cast<unsigned long long>(int()));
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
    details::Multiply(*this, n, *this);
    return *this;
  }

  inline FixedPoint &operator/=(const FixedPoint &n)
  {
    FixedPoint temp;
    *this = details::Divide(*this, n, temp);
    return *this;
  }

  inline FixedPoint &operator>>=(const FixedPoint &n)
  {
    data_ >>= n.integer();
    return *this;
  }

  inline FixedPoint &operator<<=(const FixedPoint &n)
  {
    data_ <<= n.integer();
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

  static inline FixedPoint FromBase(Type n)
  {
    return FixedPoint(n, NoScale());
  }

  static inline FixedPoint Exp(FixedPoint &x)
  {
    std::cerr << "x = " << x << std::endl;
    // Find integer k and r ∈ [-0.5, 0.5) such as: x = k*ln2 + r
    // Then exp(x) = 2^k * e^r
    FixedPoint k = x / FixedPoint::LN2;
    std::cerr << "k = " << k << std::endl;
    k = k.floor();
    std::cerr << "k = " << k << std::endl;

    FixedPoint r = x - k*FixedPoint::LN2;
    std::cerr << "r = " << r << std::endl;
    FixedPoint e1{One};
    std::cerr << "e1 = " << e1 << std::endl;
    e1 <<= k;
    std::cerr << "e1 = " << e1 << std::endl;

    // The fractional part, we take the first 3 terms of the Taylor approximation
    /*FixedPoint r2 = FixedPoint(0.5) * r * r;
    //std::cerr << "r2 = " << r2 << std::endl;
    FixedPoint r3 = FixedPoint(1.0/3.0) * r2 * r;
    //std::cerr << "r3 = " << r3 << std::endl;
    FixedPoint r4 = FixedPoint(1.0/4.0) * r3 * r;
    //std::cerr << "r4 = " << r4 << std::endl;
    FixedPoint r5 = FixedPoint(1.0/5.0) * r4 * r;
    //std::cerr << "r5 = " << r5 << std::endl;
    FixedPoint r6 = FixedPoint(1.0/6.0) * r5 * r;
    std::cerr << "r6 = " << r6 << std::endl;
    FixedPoint r7 = FixedPoint(1.0/7.0) * r6 * r;
    std::cerr << "r7 = " << r7 << std::endl;
    FixedPoint r8 = FixedPoint(1.0/8.0) * r7 * r;
    std::cerr << "r8 = " << r8 << std::endl;
    FixedPoint r9 = FixedPoint(1.0/9.0) * r8 * r;
    std::cerr << "r9 = " << r9 << std::endl;*/
    //FixedPoint e2 = FixedPoint(1) + r + r2 + r3 + r4 + r5/* + r6 + r7 + r8 + r9*/;
    //std::cerr << "e2 = " << e2 << std::endl;
    // https://en.wikipedia.org/wiki/Pad%C3%A9_table
    FixedPoint r2 = r*r;
    std::cerr << "r2 = " << r2 << std::endl;
    FixedPoint r3 = r2*r;
    std::cerr << "r3 = " << r3 << std::endl;
    FixedPoint r4 = r3*r;
    std::cerr << "r4 = " << r4 << std::endl;
    //FixedPoint P = FixedPoint(120) + FixedPoint(60)*r + FixedPoint(12)*r2 + r3;
    FixedPoint P = FixedPoint(1680) + FixedPoint(840)*r + FixedPoint(180)*r2 + FixedPoint(20)*r3 + r4;
    std::cerr << "P = " << P << std::endl;
    //FixedPoint Q = FixedPoint(120) - FixedPoint(60)*r + FixedPoint(12)*r2 - r3;
    FixedPoint Q = FixedPoint(1680) - FixedPoint(840)*r + FixedPoint(180)*r2 - FixedPoint(20)*r3 + r4;
    std::cerr << "Q = " << Q << std::endl;
    FixedPoint e2 = P/Q;
    std::cerr << "e2 = " << e2 << std::endl;

    return e1 * e2;
  }
private:
  // this makes it simpler to create a fixed point object from
  // a native type without scaling
  // use "FixedPoint::from_base" in order to perform this.
  struct NoScale
  {
  };

  inline FixedPoint(Type n, const NoScale &)
    : data_(n)
  {}

private:
  Type data_;  // the value to be stored
};

template <std::size_t I, std::size_t F>
std::ostream &operator<<(std::ostream &s, FixedPoint<I, F> const &n)
{
  std::ios_base::fmtflags f(s.flags());
  s << std::dec << std::setprecision(14);
  s << (double)(n);
  s.flags(f);
  return s;
}

template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::One{1}; /* e */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::E{2.718281828459045235360287471352662498}; /* e */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::LOG2E{1.442695040888963407359924681001892137}; /* log_2 e */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::LOG10E{0.434294481903251827651128918916605082}; /* log_10 e */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::LN2{0.693147180559945309417232121458176568}; /* log_e 2 */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::LN10{2.302585092994045684017991454684364208}; /* log_e 10 */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::PI{3.141592653589793238462643383279502884}; /* pi */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::PI_2{1.570796326794896619231321691639751442}; /* pi/2 */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::PI_4{0.785398163397448309615660845819875721}; /* pi/4 */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::INV_PI{0.318309886183790671537767526745028724}; /* 1/pi */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::INV2_PI{0.636619772367581343075535053490057448}; /* 2/pi */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::INV2_SQRTPI{1.128379167095512573896158903121545172}; /* 2/sqrt(pi) */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::SQRT2{1.414213562373095048801688724209698079}; /* sqrt(2) */
template <std::size_t I, std::size_t F> const FixedPoint<I, F> FixedPoint<I, F>::INV_SQRT2{0.707106781186547524400844362104849039}; /* 1/sqrt(2) */


}  // namespace fixed_point
}  // namespace fetch
