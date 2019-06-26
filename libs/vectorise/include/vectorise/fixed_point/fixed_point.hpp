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
#include "vectorise/platform.hpp"

#include <cassert>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>

namespace fetch {
namespace fixed_point {

template <std::uint16_t I, std::uint16_t F>
class FixedPoint;

using fp32_t  = FixedPoint<16, 16>;
using fp64_t  = FixedPoint<32, 32>;
using fp128_t = FixedPoint<64, 64>;

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
  static constexpr bool          is_valid = true;
  static constexpr std::uint16_t size     = 128;
  using ValueType                         = __int128_t;
  using UnsignedType                      = __uint128_t;
  using SignedType                        = __int128_t;
  // Commented out, when we need to implement FixedPoint<128,128> fully, we will deal with that
  // then.
  // using NextSize                        = TypeFromSize<256>;
};
#endif

// 64 bit implementation
template <>
struct TypeFromSize<64>
{
  static constexpr bool          is_valid  = true;
  static constexpr std::uint16_t size      = 64;
  using ValueType                          = int64_t;
  using UnsignedType                       = uint64_t;
  using SignedType                         = int64_t;
  using NextSize                           = TypeFromSize<128>;
  static constexpr std::uint16_t decimals  = 9;
  static constexpr ValueType     tolerance = 0x200;                 // 0.00000012
  static constexpr ValueType     max_exp   = 0x000000157cd0e714LL;  // 21.48756260
};

// 32 bit implementation
template <>
struct TypeFromSize<32>
{
  static constexpr bool          is_valid  = true;
  static constexpr std::uint16_t size      = 32;
  using ValueType                          = int32_t;
  using UnsignedType                       = uint32_t;
  using SignedType                         = int32_t;
  using NextSize                           = TypeFromSize<64>;
  static constexpr std::uint16_t decimals  = 4;
  static constexpr ValueType     tolerance = 0x15;         // 0.0003
  static constexpr ValueType     max_exp   = 0x000a65b9L;  // 10.3974
};

// 16 bit implementation
template <>
struct TypeFromSize<16>
{
  static constexpr bool          is_valid = true;
  static constexpr std::uint16_t size     = 16;
  using ValueType                         = int16_t;
  using UnsignedType                      = uint16_t;
  using SignedType                        = int16_t;
  using NextSize                          = TypeFromSize<32>;
};

// 8 bit implementation
template <>
struct TypeFromSize<8>
{
  static constexpr bool          is_valid = true;
  static constexpr std::uint16_t size     = 8;
  using ValueType                         = int8_t;
  using UnsignedType                      = uint8_t;
  using SignedType                        = int8_t;
  using NextSize                          = TypeFromSize<16>;
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
inline FixedPoint<I, F> Divide(FixedPoint<I, F> const &numerator,
                               FixedPoint<I, F> const &denominator,
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
inline void Multiply(FixedPoint<I, F> const &lhs, FixedPoint<I, F> const &rhs,
                     FixedPoint<I, F> &result)
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
constexpr inline int32_t HighestSetBit(T n_input)
{
  uint64_t n = static_cast<uint64_t>(n_input);

  if (n == 0)
  {
    return 0;
  }

  return static_cast<int32_t>((sizeof(uint64_t) * 8)) -
         static_cast<int32_t>(platform::CountLeadingZeroes64(n));
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
    ONE_MASK        = Type(1) << FRACTIONAL_BITS
  };

  ////////////////////////
  /// Constants/Limits ///
  ////////////////////////

  static constexpr Type          SMALLEST_FRACTION{1};
  static constexpr Type          LARGEST_FRACTION{FRACTIONAL_MASK};
  static constexpr Type          MAX_INT{Type(FRACTIONAL_MASK >> 1) << FRACTIONAL_BITS};
  static constexpr Type          MIN_INT{INTEGER_MASK & ((Type(1) << (TOTAL_BITS - 1)))};
  static constexpr Type          MAX{MAX_INT | LARGEST_FRACTION};
  static constexpr Type          MIN{MIN_INT | LARGEST_FRACTION};
  static constexpr std::uint16_t DECIMAL_DIGITS{BaseTypeInfo::decimals};

  static FixedPoint const TOLERANCE;
  static FixedPoint const _0; /* 0 */
  static FixedPoint const _1; /* 1 */

  struct MathConstants
  {
    FixedPoint SMALLEST_FRACTION;
    FixedPoint E;              /* e */
    FixedPoint LOG2E;          /* log_2 e */
    FixedPoint LOG210;         /* log_2 10 */
    FixedPoint LOG10E;         /* log_10 e */
    FixedPoint LN2;            /* log_e 2 */
    FixedPoint LN10;           /* log_e 10 */
    FixedPoint PI;             /* pi */
    FixedPoint PI_2;           /* pi/2 */
    FixedPoint PI_4;           /* pi/4 */
    FixedPoint INV_PI;         /* 1/pi */
    FixedPoint TWO_INV_PI;     /* 2/pi */
    FixedPoint TWO_INV_SQRTPI; /* 2/sqrt(pi) */
    FixedPoint SQRT2;          /* sqrt(2) */
    FixedPoint INV_SQRT2;      /* 1/sqrt(2) */
    FixedPoint MAX_EXP;
    FixedPoint MIN_EXP;
    FixedPoint MAX;
    FixedPoint MIN;
    FixedPoint NaN;
    FixedPoint POSITIVE_INFINITY;
    FixedPoint NEGATIVE_INFINITY;
  };
  static struct MathConstants       GenerateConstants();
  static struct MathConstants const Constants;

  ///////////////////////////
  /// State of operations ///
  ///////////////////////////

  enum
  {
    STATE_OK               = 0,
    STATE_NAN              = 1 << 0,
    STATE_DIVISION_BY_ZERO = 1 << 1,
    STATE_UNDERFLOW        = 1 << 2,
    STATE_OVERFLOW         = 1 << 3,
    STATE_INFINITY         = 1 << 4,
  };
  static uint32_t fp_state;

  static constexpr void stateClear();
  static constexpr bool isState(const uint32_t state);
  static constexpr bool isStateNaN();
  static constexpr bool isStateUnderflow();
  static constexpr bool isStateOverflow();
  static constexpr bool isStateInfinity();
  static constexpr bool isStateDivisionByZero();

  ////////////////////
  /// constructors ///
  ////////////////////

  constexpr FixedPoint() = default;

  /**
   * Templated constructor existing only for T is an integer and assigns data
   * @tparam T any integer type
   * @param n integer value to set FixedPoint to
   */
  template <typename T>
  constexpr explicit FixedPoint(T n, meta::IfIsInteger<T> * = nullptr);
  template <typename T>
  constexpr explicit FixedPoint(T n, meta::IfIsFloat<T> * = nullptr);
  constexpr FixedPoint(FixedPoint const &o);
  constexpr FixedPoint(const Type &integer, const UnsignedType &fraction);

  ///////////////////
  /// conversions ///
  ///////////////////

  constexpr Type              Integer() const;
  constexpr Type              Fraction() const;
  static constexpr FixedPoint Floor(FixedPoint const &o);
  static constexpr FixedPoint Round(FixedPoint const &o);
  static constexpr FixedPoint FromBase(Type n);

  /////////////////////////
  /// casting operators ///
  /////////////////////////

  explicit operator double() const;
  explicit operator float() const;
  template <typename T>
  explicit operator T() const;

  /////////////////
  /// operators ///
  /////////////////

  constexpr FixedPoint &operator=(FixedPoint const &o);
  template <typename T>
  constexpr meta::IfIsInteger<T, FixedPoint> &operator=(T const &n);
  template <typename T>
  constexpr meta::IfIsFloat<T, FixedPoint> &operator=(T const &n);

  ///////////////////////////////////////////////////
  /// comparison operators for FixedPoint objects ///
  ///////////////////////////////////////////////////

  constexpr bool operator==(FixedPoint const &o) const;
  constexpr bool operator!=(FixedPoint const &o) const;
  constexpr bool operator<(FixedPoint const &o) const;
  constexpr bool operator>(FixedPoint const &o) const;
  constexpr bool operator<=(FixedPoint const &o) const;
  constexpr bool operator>=(FixedPoint const &o) const;

  ////////////////////////////////////////////////
  /// comparison operators against other types ///
  ////////////////////////////////////////////////

  template <typename OtherType>
  constexpr bool operator==(OtherType const &o) const;
  template <typename OtherType>
  constexpr bool operator!=(OtherType const &o) const;
  template <typename OtherType>
  constexpr bool operator<(OtherType const &o) const;
  template <typename OtherType>
  constexpr bool operator>(OtherType const &o) const;
  template <typename OtherType>
  constexpr bool operator<=(OtherType const &o) const;
  template <typename OtherType>
  constexpr bool operator>=(OtherType const &o) const;

  ///////////////////////
  /// unary operators ///
  ///////////////////////

  constexpr bool        operator!() const;
  constexpr FixedPoint  operator~() const;
  constexpr FixedPoint  operator-() const;
  constexpr FixedPoint &operator++();
  constexpr FixedPoint &operator--();

  //////////////////////////////
  /// math and bit operators ///
  //////////////////////////////

  constexpr FixedPoint  operator+(FixedPoint const &n) const;
  constexpr FixedPoint  operator-(FixedPoint const &n) const;
  constexpr FixedPoint  operator*(FixedPoint const &n) const;
  constexpr FixedPoint  operator/(FixedPoint const &n) const;
  constexpr FixedPoint  operator&(FixedPoint const &n) const;
  constexpr FixedPoint  operator|(FixedPoint const &n) const;
  constexpr FixedPoint  operator^(FixedPoint const &n) const;
  constexpr FixedPoint &operator+=(FixedPoint const &n);
  constexpr FixedPoint &operator-=(FixedPoint const &n);
  constexpr FixedPoint &operator*=(FixedPoint const &n);
  constexpr FixedPoint &operator/=(FixedPoint const &n);
  constexpr FixedPoint &operator&=(FixedPoint const &n);
  constexpr FixedPoint &operator|=(FixedPoint const &n);
  constexpr FixedPoint &operator^=(FixedPoint const &n);
  constexpr FixedPoint &operator>>=(FixedPoint const &n);
  constexpr FixedPoint &operator<<=(FixedPoint const &n);
  template <typename T>
  constexpr FixedPoint operator+(T const &n) const;
  template <typename T>
  constexpr FixedPoint operator-(T const &n) const;
  template <typename T>
  constexpr FixedPoint operator*(T const &n) const;
  template <typename T>
  constexpr FixedPoint operator/(T const &n) const;
  template <typename T>
  constexpr FixedPoint operator&(T const &n) const;
  template <typename T>
  constexpr FixedPoint operator|(T const &n) const;
  template <typename T>
  constexpr FixedPoint operator^(T const &n) const;
  template <typename T>
  constexpr FixedPoint &operator+=(T const &n) const;
  template <typename T>
  constexpr FixedPoint &operator-=(T const &n) const;
  template <typename T>
  constexpr FixedPoint &operator&=(T const &n) const;
  template <typename T>
  constexpr FixedPoint &operator|=(T const &n) const;
  template <typename T>
  constexpr FixedPoint &operator^=(T const &n) const;
  template <typename T>
  constexpr FixedPoint &operator*=(T const &n) const;
  template <typename T>
  constexpr FixedPoint &operator/=(T const &n) const;
  constexpr FixedPoint &operator<<=(const int &n);
  constexpr FixedPoint &operator>>=(const int &n);

  ///////////////////////////
  /// NaN/Infinity checks ///
  ///////////////////////////

  static constexpr bool       isNaN(FixedPoint const &x);
  static constexpr bool       isPosInfinity(FixedPoint const &x);
  static constexpr bool       isNegInfinity(FixedPoint const &x);
  static constexpr bool       isInfinity(FixedPoint const &x);
  static constexpr FixedPoint infinity(bool const isPositive);

  ////////////
  /// swap ///
  ////////////

  constexpr void Swap(FixedPoint &rhs);

  /////////////////////
  /// Getter/Setter ///
  /////////////////////

  constexpr Type Data() const;
  constexpr void SetData(Type const n) const;

  ///////////////////////////////////////////////////////////////////
  /// FixedPoint implementations of common mathematical functions ///
  ///////////////////////////////////////////////////////////////////

  static constexpr FixedPoint Remainder(FixedPoint const &x, FixedPoint const &y);
  static constexpr FixedPoint Fmod(FixedPoint const &x, FixedPoint const &y);
  static constexpr FixedPoint Abs(FixedPoint const &x);
  static constexpr FixedPoint Sign(FixedPoint const &x);
  static constexpr FixedPoint Exp(FixedPoint const &x);
  static constexpr FixedPoint Log2(FixedPoint const &x);
  static constexpr FixedPoint Log(FixedPoint const &x);
  static constexpr FixedPoint Log10(FixedPoint const &x);
  static constexpr FixedPoint Sqrt(FixedPoint const &x);
  static constexpr FixedPoint Pow(FixedPoint const &x, FixedPoint const &y);
  static constexpr FixedPoint Sin(FixedPoint const &x);
  static constexpr FixedPoint Cos(FixedPoint const &x);
  static constexpr FixedPoint Tan(FixedPoint const &x);
  static constexpr FixedPoint ASin(FixedPoint const &x);
  static constexpr FixedPoint ACos(FixedPoint const &x);
  static constexpr FixedPoint ATan(FixedPoint const &x);
  static constexpr FixedPoint ATan2(FixedPoint const &y, FixedPoint const &x);
  static constexpr FixedPoint SinH(FixedPoint const &x);
  static constexpr FixedPoint CosH(FixedPoint const &x);
  static constexpr FixedPoint TanH(FixedPoint const &x);
  static constexpr FixedPoint ASinH(FixedPoint const &x);
  static constexpr FixedPoint ACosH(FixedPoint const &x);
  static constexpr FixedPoint ATanH(FixedPoint const &x);

private:
  Type data_{0};  // the value to be stored

  static std::function<FixedPoint(FixedPoint const &x)> SinPi2QuadrantFuncs[4];
  static std::function<FixedPoint(FixedPoint const &x)> CosPi2QuadrantFuncs[4];

  // this makes it simpler to create a fixed point object from
  // a native type without scaling
  struct NoScale
  {
  };

  constexpr FixedPoint(Type n, const NoScale &)
    : data_(n)
  {}

  /**
   * helper function that checks no bit overflow when shifting
   * @tparam T the input original type
   * @param n the value of the datum
   * @return true if there is no overflow, false otherwise
   */
  template <typename T>
  static constexpr bool CheckNoOverflow(T n)
  {
    if (Type(n) <= MAX && Type(n) >= MIN)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  /**
   * helper function that checks no rounding error when casting
   * @tparam T
   * @param n
   * @return true if there is no overflow, false otherwise
   */
  template <typename T>
  static constexpr bool CheckNoRounding()
  {
    // sufficient bits to guarantee no rounding
    if (std::numeric_limits<T>::max_digits10 < DECIMAL_DIGITS)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  static constexpr int ReduceSqrt(FixedPoint &x)
  {
    /* Given x, find k such as x = 2^{2*k} * y, where 1 < y < 4
     */
    int k = 0;
    while (x > 4)
    {
      k++;
      x >>= 2;
    }
    if (x < _1)
    {
      while (x < _1)
      {
        k++;
        x <<= 2;
      }
      k = -k;
    }
    return k;
  }

  static constexpr FixedPoint SinPi2(FixedPoint const &r)
  {
    assert(r <= Constants.PI_2);

    if (r > Constants.PI_4)
    {
      return std::move(CosPi2(r - Constants.PI_2));
    }

    return std::move(SinApproxPi4(r));
  }

  static constexpr FixedPoint CosPi2(FixedPoint const &r)
  {
    assert(r <= Constants.PI_2);

    if (r > Constants.PI_4)
    {
      return std::move(SinPi2(Constants.PI_2 - r));
    }

    return std::move(CosApproxPi4(r));
  }

  static constexpr FixedPoint<16, 16> SinApproxPi4(FixedPoint<16, 16> const &r)
  {
    assert(r <= Constants.PI_4);

    FixedPoint r2 = r * r;
    FixedPoint r3 = r2 * r;
    FixedPoint r4 = r3 * r;
    FixedPoint Q00{5880};
    FixedPoint P   = r * Q00 - r3 * 620;
    FixedPoint Q   = Q00 + r2 * 360 + r4 * 11;
    FixedPoint sin = P / Q;

    return std::move(sin);
  }

  static constexpr FixedPoint<32, 32> SinApproxPi4(FixedPoint<32, 32> const &r)
  {
    assert(r <= Constants.PI_4);

    FixedPoint r2 = r * r;
    FixedPoint r3 = r2 * r;
    FixedPoint r4 = r3 * r;
    FixedPoint r5 = r4 * r;
    FixedPoint Q00{166320};
    FixedPoint P   = r * Q00 - r3 * 22260 + r5 * 551;
    FixedPoint Q   = Q00 + r2 * 5460 + r4 * 75;
    FixedPoint sin = P / Q;

    return std::move(sin);
  }

  static constexpr FixedPoint CosApproxPi4(FixedPoint const &r)
  {
    assert(r <= Constants.PI_4);

    FixedPoint r2 = r * r;
    FixedPoint r4 = r2 * r2;
    FixedPoint Q00{15120};
    FixedPoint P   = Q00 - r2 * 6900 + r4 * 313;
    FixedPoint Q   = Q00 + r2 * 660 + r4 * 13;
    FixedPoint cos = P / Q;

    return std::move(cos);
  }
};

template <std::uint16_t I, std::uint16_t F>
std::function<FixedPoint<I, F>(FixedPoint<I, F> const &x)>
    FixedPoint<I, F>::SinPi2QuadrantFuncs[4] = {
        [](FixedPoint<I, F> const &x) { return FixedPoint<I, F>::SinPi2(x); },
        [](FixedPoint<I, F> const &x) { return FixedPoint<I, F>::CosPi2(x); },
        [](FixedPoint<I, F> const &x) { return -FixedPoint<I, F>::SinPi2(x); },
        [](FixedPoint<I, F> const &x) { return -FixedPoint<I, F>::CosPi2(x); }};

template <std::uint16_t I, std::uint16_t F>
std::function<FixedPoint<I, F>(FixedPoint<I, F> const &x)>
    FixedPoint<I, F>::CosPi2QuadrantFuncs[4] = {
        [](FixedPoint<I, F> const &x) { return FixedPoint<I, F>::CosPi2(x); },
        [](FixedPoint<I, F> const &x) { return -FixedPoint<I, F>::SinPi2(x); },
        [](FixedPoint<I, F> const &x) { return -FixedPoint<I, F>::CosPi2(x); },
        [](FixedPoint<I, F> const &x) { return FixedPoint<I, F>::SinPi2(x); }};

template <std::uint16_t I, std::uint16_t F>
uint32_t FixedPoint<I, F>::fp_state{FixedPoint<I, F>::STATE_OK};

template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::SMALLEST_FRACTION;
template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::LARGEST_FRACTION;
template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::MAX_INT;
template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::MIN_INT;
template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::MAX;
template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::MIN;
template <std::uint16_t I, std::uint16_t F>
constexpr std::uint16_t FixedPoint<I, F>::DECIMAL_DIGITS;

template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::_0{0}; /* 0 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::_1{1}; /* 1 */
template <std::uint16_t I, std::uint16_t F>

FixedPoint<I, F> const FixedPoint<I, F>::TOLERANCE(
    0, FixedPoint<I, F>::BaseTypeInfo::tolerance); /* 0 */

template <std::uint16_t I, std::uint16_t F>
struct FixedPoint<I, F>::MathConstants FixedPoint<I, F>::GenerateConstants()
{
  struct FixedPoint<I, F>::MathConstants constants;

  constants.SMALLEST_FRACTION = FixedPoint<I, F>(0, FixedPoint::SMALLEST_FRACTION);
  constants.E                 = 2.718281828459045235360287471352662498; /* e */
  constants.LOG2E             = 1.442695040888963407359924681001892137; /* log_2 e */
  constants.LOG210            = 3.3219280948874;                        /* log_2 10 */
  constants.LOG10E            = 0.434294481903251827651128918916605082; /* log_10 e */
  constants.LN2               = 0.693147180559945309417232121458176568; /* log_e 2 */
  constants.LN10              = 2.302585092994045684017991454684364208; /* log_e 10 */
  constants.PI                = 3.141592653589793238462643383279502884; /* pi */
  constants.PI_2              = 1.570796326794896619231321691639751442; /* pi/2 */
  constants.PI_4              = 0.785398163397448309615660845819875721; /* pi/4 */
  constants.INV_PI            = 0.318309886183790671537767526745028724; /* 1/pi */
  constants.TWO_INV_PI        = 0.636619772367581343075535053490057448; /* 2/pi */
  constants.TWO_INV_SQRTPI    = 1.128379167095512573896158903121545172; /* 2/sqrt(pi) */
  constants.SQRT2             = 1.414213562373095048801688724209698079; /* sqrt(2) */
  constants.INV_SQRT2         = 0.707106781186547524400844362104849039; /* 1/sqrt(2) */
  constants.MAX_EXP           = FixedPoint<I, F>::FromBase(
      FixedPoint<I, F>::BaseTypeInfo::max_exp); /* maximum exponent for Exp() */
  constants.MIN_EXP = -constants.MAX_EXP;                 /* minimum exponent for Exp() */
  constants.MAX     = FixedPoint<I, F>::FromBase(FixedPoint<I, F>::MAX);
  constants.MIN     = FixedPoint<I, F>::FromBase(FixedPoint<I, F>::MIN);
  constants.NaN     = FixedPoint<I, F>::FromBase(
      FixedPoint<I, F>::Type(1) << (FixedPoint<I, F>::TOTAL_BITS - 1) | FixedPoint<I, F>::Type(1));
  constants.POSITIVE_INFINITY =
      constants.NaN | FixedPoint<I, F>::FromBase(FixedPoint<I, F>::Type(1)
                                                 << (FixedPoint<I, F>::FRACTIONAL_BITS - 1));
  constants.NEGATIVE_INFINITY =
      constants.NaN | FixedPoint<I, F>::FromBase(FixedPoint<I, F>::Type(3)
                                                 << (FixedPoint<I, F>::FRACTIONAL_BITS - 2));

  return constants;
}

template <std::uint16_t I, std::uint16_t F>
struct FixedPoint<I, F>::MathConstants const FixedPoint<I, F>::Constants =
    FixedPoint<I, F>::GenerateConstants();

template <std::uint16_t I, std::uint16_t F>
std::ostream &operator<<(std::ostream &s, FixedPoint<I, F> const &n)
{
  std::ios_base::fmtflags f(s.flags());
  s << std::setprecision(F / 4);
  s << std::fixed;
  if (FixedPoint<I, F>::isNaN(n))
  {
    s << "NaN";
  }
  else if (FixedPoint<I, F>::isPosInfinity(n))
  {
    s << "+∞";
  }
  else if (FixedPoint<I, F>::isNegInfinity(n))
  {
    s << "-∞";
  }
  else
  {
    s << double(n);
  }
  s << " (0x" << std::hex << n.Data() << ")";
  s.flags(f);
  return s;
}

///////////////////////////
/// State of operations ///
///////////////////////////

template <std::uint16_t I, std::uint16_t F>
constexpr void FixedPoint<I, F>::stateClear()
{
  fp_state = 0;
}

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isState(const uint32_t state)
{
  if (fp_state & state)
  {
    return true;
  }
  else
  {
    return false;
  }
}

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isStateNaN()
{
  return isState(STATE_NAN);
}

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isStateUnderflow()
{
  return isState(STATE_UNDERFLOW);
}

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isStateOverflow()
{
  return isState(STATE_OVERFLOW);
}

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isStateInfinity()
{
  return isState(STATE_INFINITY);
}

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isStateDivisionByZero()
{
  return isState(STATE_DIVISION_BY_ZERO);
}

////////////////////
/// constructors ///
////////////////////

/**
 * Templated constructor when T is an integer type
 * @tparam T any integer type
 * @param n integer value to set FixedPoint to
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F>::FixedPoint(T n, meta::IfIsInteger<T> *)
  : data_{static_cast<typename FixedPoint<I, F>::Type>(n)}
{
  if (CheckNoOverflow(n))
  {
    fp_state |= STATE_OVERFLOW;
  }
  Type s    = (data_ < 0) - (data_ > 0);
  Type abs_ = s * data_;
  abs_ <<= FRACTIONAL_BITS;
  data_ = s * abs_;
}

/**
 * Templated constructor only when T is a float/double type
 * @tparam T any float/double type
 * @param n float/double value to set FixedPoint to
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F>::FixedPoint(T n, meta::IfIsFloat<T> *)
  : data_(static_cast<typename FixedPoint<I, F>::Type>(n * ONE_MASK))
{
  if (CheckNoOverflow(n * ONE_MASK))
  {
    fp_state |= STATE_OVERFLOW;
  }
  //    assert(CheckNoRounding<T>());
}

/**
 * Copy constructor
 * @param o other FixedPoint object to copy
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F>::FixedPoint(FixedPoint<I, F> const &o)
  : data_{o.data_}
{}

/**
 * Templated constructor taking 2 arguments, integer and fraction part
 * @param integer
 * @param fraction
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F>::FixedPoint(const typename FixedPoint<I, F>::Type &        integer,
                                       const typename FixedPoint<I, F>::UnsignedType &fraction)
  : data_{(INTEGER_MASK & (Type(integer) << FRACTIONAL_BITS)) | Type(fraction & FRACTIONAL_MASK)}
{}

///////////////////
/// conversions ///
///////////////////

/**
 * Method to return the integer part of the FixedPoint object
 * @return the integer part of the FixedPoint object
 */
template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::Integer() const
{
  if (isNaN(*this))
  {
    fp_state |= STATE_NAN;
  }
  return Type((data_ & INTEGER_MASK) >> FRACTIONAL_BITS);
}

/**
 * Method to return the fraction part of the FixedPoint object
 * @return the fraction part of the FixedPoint object
 */
template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::Fraction() const
{
  if (isNaN(*this))
  {
    fp_state |= STATE_NAN;
  }
  return (data_ & FRACTIONAL_MASK);
}

/**
 * Same as Integer()
 * @param the FixedPoint object to use Floor() on
 * @return the integer part of the FixedPoint object
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Floor(FixedPoint<I, F> const &o)
{
  if (isNaN(o))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  return std::move(FixedPoint{o.Integer()});
}

/**
 * Return the nearest greater integer to the FixedPoint number o
 * @param the FixedPoint object to use Round() on
 * @return the nearest greater integer to the FixedPoint number o
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Round(FixedPoint<I, F> const &o)
{
  if (isNaN(o))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  return std::move(Floor(o + FixedPoint{0.5}));
}

/**
 * Take a raw primitive of Type and convert it to a FixedPoint object
 * @param the primitive to convert to FixedPoint
 * @return the converted FixedPoint
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::FromBase(typename FixedPoint<I, F>::Type n)
{
  return FixedPoint(n, NoScale());
}

/////////////////////////
/// casting operators ///
/////////////////////////

/**
 * Cast the FixedPoint object to a double primitive
 * @return the cast FixedPoint object to double
 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F>::operator double() const
{
  return (static_cast<double>(data_) / ONE_MASK);
}

/**
 * Cast the FixedPoint object to a float primitive
 * @return the cast FixedPoint object to float
 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F>::operator float() const
{
  return (static_cast<float>(data_) / ONE_MASK);
}

/**
 * Cast the FixedPoint object to an integer primitive
 * @return the cast FixedPoint object to an integer primitive
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
FixedPoint<I, F>::operator T() const
{
  return (static_cast<T>(Integer()));
}

/////////////////
/// operators ///
/////////////////

/**
 * Assignment operator, same as copy constructor
 * @param the FixedPoint object to assign from
 * @return copies the given FixedPoint object o
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator=(FixedPoint<I, F> const &o)
{
  if (isNaN(o))
  {
    fp_state |= STATE_NAN;
  }
  data_ = o.data_;
  return *this;
}

/**
 * Assignment operator for primitive integer types
 * @param the primitive to assign the FixedPoint object from
 * @return copies the given primitive integer n
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr meta::IfIsInteger<T, FixedPoint<I, F>> &FixedPoint<I, F>::operator=(T const &n)
{
  data_ = {static_cast<Type>(n) << static_cast<Type>(FRACTIONAL_BITS)};
  if (CheckNoOverflow(n))
  {
    fp_state |= STATE_OVERFLOW;
  }
  return *this;
}

/**
 * Assignment operator for primitive float types
 * @param the primitive to assign the FixedPoint object from
 * @return copies the given primitive float n
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr meta::IfIsFloat<T, FixedPoint<I, F>> &FixedPoint<I, F>::operator=(T const &n)
{
  data_ = static_cast<typename FixedPoint<I, F>::Type>(n * ONE_MASK);
  if (CheckNoOverflow(n * ONE_MASK))
  {
    fp_state |= STATE_OVERFLOW;
  }
  return *this;
}

///////////////////////////////////////////////////
/// comparison operators for FixedPoint objects ///
///////////////////////////////////////////////////

/**
 * Equality comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if objects are equal, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator==(FixedPoint const &o) const
{
  if (isNaN(*this) || isNaN(o))
  {
    return false;
  }
  else
  {
    return (data_ == o.Data());
  }
}

/**
 * Inequality comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if objects are unequal, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator!=(FixedPoint const &o) const
{
  if (isNaN(*this) || isNaN(o))
  {
    return true;
  }
  else
  {
    return (data_ != o.Data());
  }
}

/**
 * Less than comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is less than o, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator<(FixedPoint const &o) const
{
  if (isNaN(*this) || isNaN(o))
  {
    return false;
  }
  else if (isNegInfinity(*this))
  {
    // Negative infinity is always smaller than all other quantities except itself
    if (isNegInfinity(o))
    {
      return false;
    }
    else
    {
      return true;
    }
  }
  else if (isPosInfinity(*this))
  {
    // Positive infinity is never smaller than any other quantity
    return false;
  }
  else
  {
    if (isNegInfinity(o))
    {
      return false;
    }
    else if (isPosInfinity(o))
    {
      return true;
    }
    else
    {
      return (data_ < o.Data());
    }
  }
}

/**
 * Less than or equal comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is less than or equal to o, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator<=(FixedPoint const &o) const
{
  return (*this < o) || (*this == o);
}

/**
 * Greater than comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is greater than o, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator>(FixedPoint const &o) const
{
  return (o < *this);
}

/**
 * Greater than or equal comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is greater than or equal to o, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator>=(FixedPoint const &o) const
{
  return (o < *this) || (*this == o);
}

////////////////////////////////////////////////
/// comparison operators against other types ///
////////////////////////////////////////////////

/**
 * Equality comparison operator, note, NaN objects are never equal to each other
 * @param the primitive object to compare to
 * @return true if objects are equal, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator==(OtherType const &o) const
{
  return (*this == FixedPoint(o));
}

/**
 * Inequality comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if objects are unequal, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator!=(OtherType const &o) const
{
  return (*this != FixedPoint(o));
}

/**
 * Less than comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is less than o, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator<(OtherType const &o) const
{
  return (*this < FixedPoint(o));
}

/**
 * Greater than comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is greater than o, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator>(OtherType const &o) const
{
  return (*this > FixedPoint(o));
}

/**
 * Less than or equal comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is less than or equal to o, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator<=(OtherType const &o) const
{
  return (*this <= FixedPoint(o));
}

/**
 * Greater than or equal comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is greater than o, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator>=(OtherType const &o) const
{
  return (*this >= FixedPoint(o));
}

///////////////////////
/// unary operators ///
///////////////////////

/**
 * Unary minus operator
 * @return the negative number
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator-() const
{
  if (isNaN(*this))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(*this))
  {
    fp_state |= STATE_INFINITY;
    return Constants.NEGATIVE_INFINITY;
  }
  else if (isNegInfinity(*this))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }
  FixedPoint t(*this);
  t.data_ = -t.data_;
  return std::move(t);
}

/**
 * Logical Negation operator, note, NaN objects are never equal to each other
 * @return true if object is greater than o, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator!() const
{
  return !data_;
}

/**
 * Bitwise NOT operator
 * @return the bitwise NOT of the number
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator~() const
{
  FixedPoint t(*this);
  t.data_ = ~t.data_;
  return t;
}

/**
 * Prefix increment operator, increase the number by one
 * @return the number increased by one, prefix mode
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator++()
{
  if (CheckNoOverflow(data_ + _1.Data()))
  {
    fp_state |= STATE_OVERFLOW;
  }
  data_ += ONE_MASK;
  return *this;
}

/**
 * Prefix decrement operator, decrease the number by one
 * @return the number decreased by one, prefix mode
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator--()
{
  if (CheckNoOverflow(data_ - _1.Data()))
  {
    fp_state |= STATE_OVERFLOW;
  }
  data_ -= ONE_MASK;
  return *this;
}

/////////////////////////////////////////////////
/// math operators against FixedPoint objects ///
/////////////////////////////////////////////////

/**
 * Addition operator
 * @param the FixedPoint object to add to
 * @return the sum of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator+(FixedPoint<I, F> const &n) const
{
  if (isNaN(*this) || isNaN(n))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(*this))
  {
    // Adding +∞ to -∞ gives NaN, +∞ otherwise
    if (isNegInfinity(n))
    {
      fp_state |= STATE_NAN;
      return Constants.NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      return Constants.POSITIVE_INFINITY;
    }
  }
  else if (isNegInfinity(*this))
  {
    // Adding +∞ to -∞ gives NaN, +∞ otherwise
    if (isPosInfinity(n))
    {
      fp_state |= STATE_NAN;
      return Constants.NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      return Constants.NEGATIVE_INFINITY;
    }
  }
  else
  {
    if (CheckNoOverflow(data_ + n.Data()))
    {
      fp_state |= STATE_OVERFLOW;
    }
    Type fp = data_ + n.Data();
    return FromBase(fp);
  }
}

/**
 * Subtraction operator
 * @param the FixedPoint object to subtract from
 * @return the difference of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator-(FixedPoint<I, F> const &n) const
{
  if (isNaN(*this) || isNaN(n))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(*this))
  {
    // Subtracting -∞ from +∞ gives NaN, +∞ otherwise
    if (isPosInfinity(n))
    {
      fp_state |= STATE_NAN;
      return Constants.NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      return Constants.POSITIVE_INFINITY;
    }
  }
  else if (isNegInfinity(*this))
  {
    // Subtracting -∞ from -∞ gives NaN, +∞ otherwise
    if (isNegInfinity(n))
    {
      fp_state |= STATE_NAN;
      return Constants.NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      return Constants.NEGATIVE_INFINITY;
    }
  }
  else
  {
    if (CheckNoOverflow(data_ - n.Data()))
    {
      fp_state |= STATE_OVERFLOW;
    }
    Type fp = data_ - n.Data();
    return FromBase(fp);
  }
}

/**
 * Multiplication operator
 * @param the FixedPoint object to multiply against
 * @return the product of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator*(FixedPoint<I, F> const &n) const
{
  if (isNaN(*this) || isNaN(n))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isInfinity(*this) && isInfinity(n))
  {
    fp_state |= STATE_INFINITY;
    return infinity((isPosInfinity(*this) && isPosInfinity(n)) ||
                    (isNegInfinity(*this) && isNegInfinity(n)));
  }
  else if (isInfinity(*this))
  {
    if (n == _0)
    {
      // ∞ * 0 is NaN
      fp_state |= STATE_NAN;
      return Constants.NaN;
    }
    else
    {
      // Normal number, return +/-∞ depending on the sign of n
      fp_state |= STATE_INFINITY;
      return infinity(n > _0);
    }
  }
  else if (isInfinity(n))
  {
    if (*this == _0)
    {
      // 0 * ∞ is NaN
      fp_state |= STATE_NAN;
      return Constants.NaN;
    }
    else
    {
      // Normal number, return +/-∞ depending on the sign of *this
      fp_state |= STATE_INFINITY;
      return infinity(*this > _0);
    }
  }
  NextType prod = NextType(data_) * NextType(n.Data());
  if (CheckNoOverflow(Type(prod >> FRACTIONAL_BITS)))
  {
    fp_state |= STATE_OVERFLOW;
  }
  Type fp = Type(prod >> FRACTIONAL_BITS);
  return FromBase(fp);
}

/**
 * Division operator
 * @param the FixedPoint object to divide against
 * @return the division of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator/(FixedPoint<I, F> const &n) const
{
  if (isNaN(*this) || isNaN(n))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (n == _0)
  {
    if (*this == _0)
    {
      fp_state |= STATE_NAN | STATE_DIVISION_BY_ZERO;
      return Constants.NaN;
    }
    else
    {
      fp_state |= STATE_DIVISION_BY_ZERO;
      return Constants.NaN;
    }
  }
  else if (isInfinity(*this))
  {
    if (isInfinity(n))
    {
      fp_state |= STATE_NAN;
      return Constants.NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      return infinity((isPosInfinity(*this) && n > _0) || (isNegInfinity(*this) && n < 0));
    }
  }
  FixedPoint sign      = Sign(*this);
  FixedPoint abs_n     = Abs(*this);
  NextType   numerator = NextType(abs_n.Data()) << FRACTIONAL_BITS;
  NextType   quotient  = numerator / NextType(n.Data());
  return sign * FromBase(Type(quotient));
}

/**
 * Bitwise AND operator, does bitwise AND between the given FixedPoint object and self
 * @param the given FixedPoint object to AND against
 * @return the bitwise AND operation between the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator&(FixedPoint<I, F> const &n) const
{
  FixedPoint t{*this};
  t &= n;
  return std::move(t);
}

/**
 * Bitwise OR operator, does bitwise OR between the given FixedPoint object and self
 * @param the given FixedPoint object to OR against
 * @return the bitwise OR operation between the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator|(FixedPoint<I, F> const &n) const
{
  FixedPoint t{*this};
  t |= n;
  return std::move(t);
}

/**
 * Bitwise XOR operator, does bitwise XOR between the given FixedPoint object and self
 * @param the given FixedPoint object to XOR against
 * @return the bitwise XOR operation between the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator^(FixedPoint<I, F> const &n) const
{
  FixedPoint t{*this};
  t ^= n;
  return std::move(t);
}

/**
 * Addition assignment operator, adds given object to self
 * @param the given FixedPoint object to add to
 * @return the sum of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator+=(FixedPoint<I, F> const &n)
{
  if (CheckNoOverflow(data_ + n.Data()))
  {
    fp_state |= STATE_OVERFLOW;
  }
  data_ += n.Data();
  return *this;
}

/**
 * Subtraction assignment operator, subtracts given object from self
 * @param the given FixedPoint object to subtract from
 * @return the difference of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator-=(FixedPoint<I, F> const &n)
{
  if (CheckNoOverflow(data_ - n.Data()))
  {
    fp_state |= STATE_OVERFLOW;
  }
  data_ -= n.Data();
  return *this;
}

/**
 * Multiplication assignment operator, multiply given object to self
 * @param the given FixedPoint object to multiply against
 * @return the product of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator*=(FixedPoint<I, F> const &n)
{
  Multiply(*this, n, *this);
  return *this;
}

/**
 * Division assignment operator, divide self against given object
 * @param the given FixedPoint object to divide against
 * @return the division of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator/=(FixedPoint<I, F> const &n)
{
  FixedPoint temp;
  *this = Divide(*this, n, temp);
  return *this;
}

/**
 * Bitwise AND assignment operator, does bitwise AND between the given FixedPoint object and self
 * @param the given FixedPoint object to AND against
 * @return the bitwise AND operation between the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator&=(FixedPoint<I, F> const &n)
{
  data_ &= n.Data();
  return *this;
}

/**
 * Bitwise OR assignment operator, does bitwise OR between the given FixedPoint object and self
 * @param the given FixedPoint object to OR against
 * @return the bitwise OR operation between the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator|=(FixedPoint<I, F> const &n)
{
  data_ |= n.Data();
  return *this;
}

/**
 * Bitwise XOR assignment operator, does bitwise XOR between the given FixedPoint object and self
 * @param the given FixedPoint object to XOR against
 * @return the bitwise XOR operation between the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator^=(FixedPoint<I, F> const &n)
{
  data_ ^= n.Data();
  return *this;
}

/**
 * Shift right assignment operator, shift self object right as many bits as Integer() part of the
 * given object
 * @param the given object
 * @return the result of the shift right operation
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator>>=(FixedPoint<I, F> const &n)
{
  data_ >>= n.Integer();
  return *this;
}

/**
 * Shift left assignment operator, shift self object left as many bits as Integer() part of the
 * given object
 * @param the given object
 * @return the result of the shift left operation
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator<<=(FixedPoint<I, F> const &n)
{
  data_ <<= n.Integer();
  return *this;
}

/////////////////////////////////////////
/// math operators against primitives ///
/////////////////////////////////////////

/**
 * Addition operator
 * @param the primitive object to add to
 * @return the sum of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator+(T const &n) const
{
  return std::move(*this + FixedPoint(n));
}

/**
 * Subtraction operator
 * @param the primitive object to subtract from
 * @return the difference of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator-(T const &n) const
{
  return std::move(*this - FixedPoint(n));
}

/**
 * Multiplication operator against primitive object
 * @param the primitive number to multiply against
 * @return the product of the two numbers
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator*(T const &n) const
{
  return std::move(*this * FixedPoint(n));
}

/**
 * Division operator
 * @param the primitive number to multiply against
 * @return the division of the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator/(T const &n) const
{
  return std::move(*this / FixedPoint(n));
}

/**
 * Bitwise AND assignment operator, does bitwise AND between the given FixedPoint object and self
 * @param the given FixedPoint object to AND against
 * @return the bitwise AND operation between the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator&(T const &n) const
{
  return std::move(*this & FixedPoint(n));
}

/**
 * Bitwise OR assignment operator, does bitwise OR between the given FixedPoint object and self
 * @param the given FixedPoint object to OR against
 * @return the bitwise OR operation between the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator|(T const &n) const
{
  return std::move(*this | FixedPoint(n));
}

/**
 * Bitwise XOR assignment operator, does bitwise XOR between the given FixedPoint object and self
 * @param the given FixedPoint object to XOR against
 * @return the bitwise XOR operation between the two FixedPoint objects
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator^(T const &n) const
{
  return std::move(*this ^ FixedPoint(n));
}

/**
 * Shift right assignment operator, shift self object right as many bits as Integer() part of the
 * given object
 * @param the given object
 * @return the result of the shift right operation
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator>>=(const int &n)
{
  data_ >>= n;
  return *this;
}

/**
 * Shift left assignment operator, shift self object left as many bits as Integer() part of the
 * given object
 * @param the given object
 * @return the result of the shift left operation
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator<<=(const int &n)
{
  data_ <<= n;
  return *this;
}

///////////////////////////
/// NaN/Infinity checks ///
///////////////////////////

/**
 * Check if given FixedPoint object represents a NaN
 * @param the given object to check for NaN
 * @return true if x is NaN, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isNaN(FixedPoint<I, F> const &x)
{
  if (x.Data() == Constants.NaN.Data())
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Check if given FixedPoint object is +infinity
 * @param the given object to check for +infinity
 * @return true if x is +infinity, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isPosInfinity(FixedPoint<I, F> const &x)
{
  if (x.Data() == Constants.POSITIVE_INFINITY.Data())
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Check if given FixedPoint object is -infinity
 * @param the given object to check for -infinity
 * @return true if x is -infinity, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isNegInfinity(FixedPoint<I, F> const &x)
{
  if (x.Data() == Constants.NEGATIVE_INFINITY.Data())
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Check if given FixedPoint object is +/- infinity
 * @param the given object to check for infinity
 * @return true if x is +/- infinity, false otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isInfinity(FixedPoint<I, F> const &x)
{
  if (isPosInfinity(x) || isNegInfinity(x))
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * helper function to return +∞ is isPositive is true -∞ otherwise
 * @param true if +∞ is needed, false otherwise
 * @return +∞ is isPositive is true -∞ otherwise
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::infinity(bool const isPositive)
{
  return isPositive ? Constants.POSITIVE_INFINITY : Constants.NEGATIVE_INFINITY;
}

////////////
/// swap ///
////////////

/**
 * Swap two FixedPoint objects
 * @param the secondary object to swap self with
 */
template <std::uint16_t I, std::uint16_t F>
constexpr void FixedPoint<I, F>::Swap(FixedPoint<I, F> &rhs)
{
  Type tmp = data_;
  data_    = rhs.Data();
  rhs.SetData(tmp);
}

/////////////////////
/// Getter/Setter ///
/////////////////////

/**
 * Return the contents of the FixedPoint object
 * @return the contents of the FixedPoint object
 */
template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::Data() const
{
  return data_;
}

/**
 * Set the contents of the FixedPoint object
 * @param the new contents
 */
template <std::uint16_t I, std::uint16_t F>
constexpr void FixedPoint<I, F>::SetData(typename FixedPoint<I, F>::Type const n) const
{
  data_ = n;
}

///////////////////////////////////////////////////////////////////
/// FixedPoint implementations of common mathematical functions ///
///////////////////////////////////////////////////////////////////

/**
 * Return the remainder of the division between x/y
 * @param the numerator x
 * @param the denominator y
 * @return the result of x - Round(x/y) * y
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Remainder(FixedPoint<I, F> const &x,
                                                       FixedPoint<I, F> const &y)
{
  FixedPoint result = x / y;
  return std::move(x - Round(result) * y);
}

/**
 * Return the result Fmod of the division between x/y
 * @param the numerator x
 * @param the denominator y
 * @return the result of the Fmod operation
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Fmod(FixedPoint<I, F> const &x,
                                                  FixedPoint<I, F> const &y)
{
  FixedPoint result = Remainder(Abs(x), Abs(y));
  if (result < _0)
  {
    result += Abs(y);
  }
  return std::move(Sign(x) * result);
}

/**
 * Return the absolute value of FixedPoint number x
 * @param the given x
 * @return the absolute value of x
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Abs(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }

  return std::move(x * Sign(x));
}

/**
 * Return the sign of a FixedPoint number
 * @param the given x
 * @return 1 if x is positive or zero, -1 if x is negative
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Sign(FixedPoint<I, F> const &x)
{
  return std::move(FixedPoint{Type((x >= _0) - (x < _0))});
}

/**
 * Calculate the FixedPoint exponent e^x.
 * Because this is a FixedPoint implementation, the range is restricted:
 * for FixedPoint<16,16> the range is [-10.3974, 10.3974],
 * for FixedPoint<32,32> the range is [-21.48756260, 21.48756260]
 *
 * Special cases
 * * x is NaN    -> e^x = NaN
 * * x < MIN_EXP -> e^x = 0
 * * x > MAX_EXP -> overflow_error exception
 * * x == 1      -> e^x = e
 * * x == 0      -> e^x = 1
 * * x == -inf   -> e^(-inf) = 0
 * * x == +inf   -> e^(+inf) = +inf
 * * x < 0       -> e^x = 1/e^(-x)
 *
 * Calculation of e^x is done by doing range reduction of x
 * Find integer k and r ∈ [-0.5, 0.5) such as: x = k*ln2 + r
 * Then e^x = 2^k * e^r
 *
 * The e^r is calculated using a Pade approximant, 5th order,
 *
 *        1 + r/2 + r^2/9 + r^3/72 + r^4/1008 + r^5/30240
 * e^r = --------------------------------------------------
 *        1 - r/2 + r^2/9 - r^3/72 + r^4/1008 - r^5/30240
 *
 * You can reproduce the result in Mathematica using:
 * PadeApproximant[Exp[x], {x, 0, 5}]
 *
 * errors for x ∈ [-10, 5):
 * FixedPoint<16,16>: average: 0.000178116, max: 0.00584819
 * FixedPoint<32,32>: average: 4.97318e-09, max: 1.66689e-07
 *
 * @param the given x
 * @return the result of e^x
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Exp(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isNegInfinity(x))
  {
    return _0;
  }
  else if (isPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }
  else if (x < Constants.MIN_EXP)
  {
    return _0;
  }
  else if (x > Constants.MAX_EXP)
  {
    fp_state |= STATE_OVERFLOW;
    return Constants.MAX;
  }
  else if (x == _1)
  {
    return Constants.E;
  }
  else if (x == _0)
  {
    return _1;
  }
  else if (x < _0)
  {
    return _1 / Exp(-x);
  }

  // Find integer k and r ∈ [-0.5, 0.5) such as: x = k*ln2 + r
  // Then exp(x) = 2^k * e^r
  FixedPoint k = x / Constants.LN2;
  k            = Floor(k);

  FixedPoint r = x - k * Constants.LN2;
  FixedPoint e1{_1};
  e1 <<= k;

  FixedPoint r2 = r * r;
  FixedPoint r3 = r2 * r;
  FixedPoint r4 = r3 * r;
  FixedPoint r5 = r4 * r;
  // Multiply the coefficients as they are the same in both numerator and denominator
  r *= FixedPoint{0.5};
  r2 *= FixedPoint{0.1111111111111111};     // 1/9
  r3 *= FixedPoint{0.01388888888888889};    // 1/72
  r4 *= FixedPoint{0.0009920634920634921};  // 1/1008
  r5 *= FixedPoint{3.306878306878307e-05};  // 1/30240
  FixedPoint P  = _1 + r + r2 + r3 + r4 + r5;
  FixedPoint Q  = _1 - r + r2 - r3 + r4 - r5;
  FixedPoint e2 = P / Q;

  return std::move(e1 * e2);
}

/**
 * Calculate the FixedPoint base2 logarithm log2(x).
 *
 * Special cases
 * * x is NaN    -> log2(NaN) = NaN
 * * x == 1      -> log2(x) = 0
 * * x == 0      -> log2(x) = -infinity
 * * x < 0       -> log2(x) = NaN
 * * x == +inf   -> log2(+inf) = +inf
 *
 * Calculation of log2(x) is done by doing range reduction of x
 * Find integer k and r ∈ (sqrt(2)/2, sqrt(2)) such as: x = 2^k * r
 * Then log2(x) = k + log2(r)
 *
 * The log2(r) is calculated using a Pade approximant, 4th order,
 *
 *                (-1 + r) * (137 + r * (1762 + r * (3762 + r * (1762 + r * 137))))
 * log2(r) = --------------------------------------------------
 *            30 * (1 + r) * (1 + r * (24 + r * (76 + r * (24 + r)))) * Log(2)
 *
 * You can reproduce the result in Mathematica using:
 * PadeApproximant[Log2[x], {x, 1, 4}]
 *
 * errors for x ∈ (0, 5):
 * FixedPoint<16,16>: average: 9.38213e-06, max: 5.34362e-05
 * FixedPoint<32,32>: average: 2.43861e-09, max: 2.27512e-08
 *
 * @param the given x
 * @return the result of log2(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Log2(FixedPoint<I, F> const &x)
{
  if (x == _1)
  {
    return _0;
  }
  else if (x == _0)
  {
    fp_state |= STATE_INFINITY;
    return Constants.NEGATIVE_INFINITY;
  }
  else if (x == Constants.SMALLEST_FRACTION)
  {
    return FixedPoint{-FRACTIONAL_BITS};
  }
  else if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }
  else if (x < _0)
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }

  /* Range Reduction: find k and f such that
     x = 2^k * r,
     We can get k easily with HighestSetBit(), but we have to subtract the fractional bits to
     mark negative logarithms for numbers < 1.
  */
  FixedPoint sign = Sign(x - _1);
  FixedPoint y{x};
  if (y < _1)
  {
    y = _1 / x;
  }
  Type       k = HighestSetBit(y.Data()) - Type(FRACTIONAL_BITS);
  FixedPoint k_shifted{Type(1) << k};
  FixedPoint r = y / k_shifted;

  FixedPoint P00{137};
  FixedPoint P01{1762};  // P03 also
  FixedPoint P02{3762};
  FixedPoint P04{137};
  FixedPoint Q0{30};
  FixedPoint Q01{24};  // Q03 also
  FixedPoint Q02{76};
  FixedPoint P = (-_1 + r) * (P00 + r * (P01 + r * (P02 + r * (P01 + r * P04))));
  FixedPoint Q = Q0 * (_1 + r) * (_1 + r * (Q01 + r * (Q02 + r * (Q01 + r)))) * Constants.LN2;
  FixedPoint R = P / Q;

  return std::move(sign * (FixedPoint(k) + R));
}

/**
 * Calculate the FixedPoint natural logarithm log(x).
 * Return the Log2(x) / Log2(e)
 * @param the given x
 * @return the result of log(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Log(FixedPoint<I, F> const &x)
{
  return Log2(x) / Constants.LOG2E;
}

/**
 * Calculate the FixedPoint base10 logarithm log(x).
 * Return the Log2(x) / Log2(10)
 * @param the given x
 * @return the result of log10(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Log10(FixedPoint<I, F> const &x)
{
  return Log2(x) / Constants.LOG210;
}

/**
 * Calculate the FixedPoint square root of x sqrt(x).
 *
 * Special cases
 * * x is NaN    -> sqrt(NaN) = NaN
 * * x == 1      -> sqrt(x) = 1
 * * x == 0      -> sqrt(x) = 0
 * * x < 0       -> sqrt(x) = NaN
 * * x == +inf   -> sqrt(+inf) = +inf
 *
 * Calculation of sqrt(x) is done by doing range reduction of x
 * Find integer k and r ∈ (sqrt(2)/2, sqrt(2)) such as: x = 2^k * r
 * Then log2(x) = k + log2(r)
 *
 * The sqrt(r) is calculated using a Pade approximant, 4th order,
 *
 *            (1 + 3 * r) * (1 + 3 * r * (11 + r * (9 + r)));
 * sqrt(r) = --------------------------------------------------
 *            (3 + r) * (3 + r * (27 + r * (33 + r)));
 *
 * You can reproduce the result in Mathematica using:
 * PadeApproximant[Sqrt[x], {x, 1, 4}]
 * Afterwards we run 2 iterations of Goldsmith's algorithm as it converges faster
 * than Newton-Raphson.
 *
 * errors for x ∈ (0, 5):
 * FixedPoint<16,16>: average: 0.000863796, max: 0.00368993
 * FixedPoint<32,32>: average: 3.71316e-10, max: 1.56033e-09
 *
 * @param the given x
 * @return the result of log2(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Sqrt(FixedPoint<I, F> const &x)
{
  if (x == _1)
  {
    return _1;
  }
  else if (x == _0)
  {
    return _0;
  }
  else if (x < _0)
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }

  FixedPoint r{x};
  int        k = ReduceSqrt(r);

  if (r != _1)
  {
    // Do a Pade Approximation, 4th order around 1.
    FixedPoint P01{3};
    FixedPoint P02{11};
    FixedPoint P03{9};
    FixedPoint Q01{3};
    FixedPoint Q02{27};
    FixedPoint Q03{33};
    FixedPoint P = (_1 + P01 * r) * (_1 + P01 * r * (P02 + r * (P03 + r)));
    FixedPoint Q = (Q01 + r) * (Q01 + r * (Q02 + r * (Q03 + r)));
    FixedPoint R = P / Q;

    // Tune the approximation with 2 iterations of Goldsmith's algorithm (converges faster than
    // NR)
    FixedPoint half{0.5};
    FixedPoint y_n = _1 / R;
    FixedPoint x_n = r * y_n;
    FixedPoint h_n = half * y_n;
    FixedPoint r_n;
    r_n = half - x_n * h_n;
    x_n = x_n + x_n * r_n;
    h_n = h_n + h_n * r_n;
    r_n = half - x_n * h_n;
    x_n = x_n + x_n * r_n;
    h_n = h_n + h_n * r_n;

    // Final result should be accurate within ~1e-9.
    r = x_n;
  }

  FixedPoint twok{1};
  if (k < 0)
  {
    twok >>= -k;
  }
  else
  {
    twok <<= k;
  }

  return std::move(twok * r);
}

/**
 * Calculate the FixedPoint power x^y
 *
 * Special cases
 * * x or y is NaN    -> pow(x, y) = NaN
 * * x == 0, y == 0   -> pow(x, y) = Nan
 * * x == 0, y != 0   -> pow(x, y) = 0
 * * x any, y == 0    -> pow(x, y) = 1
 * * x < 0, y non int -> pow(x, y) = NaN
 * * x +/-inf         -> pow(x, y) =
 * * x < 0, y int     -> pow(x, y) = \prod_1^y x
 *
 * Calculation of pow(x) is done by the following formula:
 * x^y = s * Exp(y * Log(Abs(x)));
 * where s is the +1 if y is even or Sign(x) if y is odd.
 *
 * errors for x ∈ (0, 100), y ∈ (0, 10.5):
 * FixedPoint<16,16>: average: 1.49365e-06, max: 3.04673e-05
 * FixedPoint<32,32>: average: 8.45537e-12, max: 8.70098e-10
 *
 * errors for x ∈ [-10, 10), y ∈ (-4, 4):
 * FixedPoint<16,16>: average: 3.9093e-06, max: 9.15527e-06
 * FixedPoint<32,32>: average: 7.71863e-11, max: 2.25216e-10
 *
 * @param the given x
 * @return the result of log2(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Pow(FixedPoint<I, F> const &x,
                                                 FixedPoint<I, F> const &y)
{
  if (isNaN(x) || isNaN(y))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (y == _0)
  {
    return _1;
  }
  else if (y == _1)
  {
    if (isInfinity(x))
    {
      fp_state |= STATE_INFINITY;
    }
    return x;
  }
  else if (isPosInfinity(x))
  {
    if (y > _0)
    {
      fp_state |= STATE_INFINITY;
      return Constants.POSITIVE_INFINITY;
    }
    else
    {
      return _0;
    }
  }
  else if (isNegInfinity(x))
  {
    return Pow(_0, -y);
  }
  else if (x == _0)
  {
    if (y < _0)
    {
      fp_state |= STATE_NAN;
      return Constants.NaN;
    }
    else
    {
      return _0;
    }
  }
  else if (isPosInfinity(y))
  {
    if (Abs(x) > _1)
    {
      fp_state |= STATE_INFINITY;
      return Constants.POSITIVE_INFINITY;
    }
    else if (Abs(x) == _1)
    {
      return _1;
    }
    else
    {
      return _0;
    }
  }
  else if (isNegInfinity(y))
  {
    if (Abs(x) > _1)
    {
      return _0;
    }
    else if (Abs(x) == _1)
    {
      return _1;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      return Constants.POSITIVE_INFINITY;
    }
  }
  else if (y.Fraction() == 0)
  {
    FixedPoint pow{x};
    FixedPoint t{Abs(y)};
    while (--t)
    {
      pow *= x;
    }
    if (y > _0)
    {
      return pow;
    }
    else
    {
      return _1 / pow;
    }
  }
  else if (x < _0)
  {
    // Already checked the case for integer y
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }

  FixedPoint s   = _1 * ((y.Integer() + 1) & 1) + Sign(x) * (y.Integer() & 1);
  FixedPoint pow = s * Exp(y * Log(Abs(x)));
  return std::move(pow);
}

/**
 * Calculate the FixedPoint sinus of x sin(x).
 *
 * Special cases
 * * x is NaN    -> sin(x) = NaN
 * * x is +/-inf -> sin(x) = NaN
 * * x == 0      -> sin(x) = 0
 * * x < 0       -> sin(x) = -sin(-x)
 *
 * Calculation of sin(x) is done by doing range reduction of x in the range [0, 2*PI]
 * Then we further reduce x to the [0, Pi/2] quadrant and  use the respective trigonometry identity:
 *
 * x in [0, Pi/2)       ->  SinPi2(x)
 * x in [Pi/2, Pi)      ->  CosPi2(x)
 * x in [Pi, 3*Pi/2)    -> -SinPi2(x)
 * x in [3*Pi/2, 2*Pi)  -> -CosPi2(x)
 *
 * errors for x ∈ (-100 * Pi/2, 100 *Pi/2):
 * FixedPoint<16,16>: average: 0.000552292, max: 0.108399
 * FixedPoint<32,32>: average: 4.52891e-09, max: 1.38022e-06
 *
 * @param the given x
 * @return the result of sin(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Sin(FixedPoint<I, F> const &x)
{
  if (isNaN(x) || isInfinity(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (x < _0)
  {
    return -Sin(-x);
  }

  FixedPoint r = Fmod(x, Constants.PI * 2);

  if (r == _0)
  {
    return _0;
  }

  FixedPoint quadrant = Floor(r / Constants.PI_2);

  return SinPi2QuadrantFuncs[static_cast<std::size_t>(quadrant)](r - Constants.PI_2 * quadrant);
}

/**
 * Calculate the FixedPoint cosinus of x cos(x).
 *
 * Special cases
 * * x is NaN    -> cos(x) = NaN
 * * x is +/-inf -> cos(x) = NaN
 * * x == 0      -> cos(x) = 1
 *
 * Calculation of cos(x) is done by doing range reduction of x in the range [0, 2*PI]
 * Then we further reduce x to the [0, Pi/2] quadrant and  use the respective trigonometry identity:
 *
 * x in [0, Pi/2)       ->  CosPi2(x)
 * x in [Pi/2, Pi)      -> -SinPi2(x)
 * x in [Pi, 3*Pi/2)    -> -CosPi2(x)
 * x in [3*Pi/2, 2*Pi)  ->  SinPi2(x)
 *
 * errors for x ∈ (-100 * Pi/2, 100 *Pi/2):
 * FixedPoint<16,16>: average: 0.000552292, max: 0.108399
 * FixedPoint<32,32>: average: 4.52891e-09, max: 1.38022e-06
 *
 * @param the given x
 * @return the result of cos(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Cos(FixedPoint<I, F> const &x)
{
  if (isNaN(x) || isInfinity(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }

  FixedPoint r = Fmod(Abs(x), Constants.PI * 2);
  if (r == _0)
  {
    return _1;
  }

  FixedPoint quadrant = Floor(r / Constants.PI_2);

  return CosPi2QuadrantFuncs[static_cast<std::size_t>(quadrant)](r - Constants.PI_2 * quadrant);
}

/**
 * Calculate the FixedPoint tangent of x tan(x).
 *
 * Special cases
 * * x is NaN    -> sqrt(NaN) = NaN
 * * x == 1      -> sqrt(x) = 1
 * * x == 0      -> sqrt(x) = 0
 * * x < 0       -> sqrt(x) = NaN
 * * x == +inf   -> sqrt(+inf) = +inf
 *
 * Calculation of cos(x) is done by doing range reduction of x in the range [0, 2*PI]
 * The tan(r) is calculated using a Pade approximant, 6th order,
 *
 * if x < Pi/4:
 *
 *            r * (1 + r^2 * (4/33 + r^2 * 1/495))
 * tan(r) = --------------------------------------------------
 *            1 + r^2 * (5/11 + r^2 * (2/99 + r^2 * 1/10395))
 *
 * You can reproduce the result in Mathematica using:
 * PadeApproximant[Tan[x], {x, 0, 6}] and
 *
 * if Pi/4 < x < Pi/2:
 * let y = r - Pi/2
 *
 *            y * (1 + y^2 * (4/33 + y^2 * 1/495))
 * tan(r) = --------------------------------------------------
 *            1 + r + y^2 * (5/11 + y^2 * (2/99 + y^2 * 1/10395))
 *
 * You can reproduce the result in Mathematica using:
 * PadeApproximant[Tan[x], {x, Pi/2, 6}]
 *
 * errors for x ∈ (-Pi/2 + 0.01, Pi/2 - 0.01):
 * FixedPoint<16,16>: average: 0.000552292, max: 0.108399
 * FixedPoint<32,32>: average: 4.52891e-09, max: 1.38022e-06
 *
 * @param the given x
 * @return the result of tan(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Tan(FixedPoint<I, F> const &x)
{
  if (isNaN(x) || isInfinity(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (x == Constants.PI_2)
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }
  else if (x == -Constants.PI_2)
  {
    fp_state |= STATE_INFINITY;
    return Constants.NEGATIVE_INFINITY;
  }
  else if (x < _0)
  {
    return -Tan(-x);
  }

  FixedPoint r = Fmod(x, Constants.PI);
  FixedPoint P01{-0.1212121212121212};    // 4/33
  FixedPoint P02{0.00202020202020202};    // 1/495
  FixedPoint Q01{-0.4545454545454545};    // 5/11
  FixedPoint Q02{0.0202020202020202};     // 2/99
  FixedPoint Q03{-9.62000962000962e-05};  // 1/10395
  if (r <= Constants.PI_4)
  {
    FixedPoint r2 = r * r;
    FixedPoint P  = r * (_1 + r2 * (P01 + r2 * P02));
    FixedPoint Q  = _1 + r2 * (Q01 + r2 * (Q02 + r2 * Q03));
    return std::move(P / Q);
  }
  else if (r < Constants.PI_2)
  {
    FixedPoint y  = r - Constants.PI_2;
    FixedPoint y2 = y * y;
    FixedPoint P  = -(_1 + y2 * (Q01 + y2 * (Q02 + y2 * Q03)));
    FixedPoint Q  = -Constants.PI_2 + r + y2 * y * (P01 + y2 * P02);
    return std::move(P / Q);
  }
  else
  {
    return Tan(r - Constants.PI);
  }
}

/**
 * Calculate the FixedPoint arcsin of x asin(x).
 * Based on the NetBSD libm asin() implementation
 *
 * Special cases
 * * x is NaN    -> asin(x) = NaN
 * * x is +/-inf -> asin(x) = NaN
 * * |x| > 1     -> asin(x) = Nan
 * * x < 0       -> asin(x) = -asin(-x)
 *
 * errors for x ∈ (-1, 1):
 * FixedPoint<16,16>: average: 1.76928e-05, max: 0.000294807
 * FixedPoint<32,32>: average: 2.62396e-10, max: 1.87484e-09
 *
 * @param the given x
 * @return the result of asin(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ASin(FixedPoint<I, F> const &x)
{
  if (isNaN(x) || isInfinity(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (x < _0)
  {
    return -ASin(-x);
  }
  else if (x > _1)
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }

  FixedPoint P00{1.66666666666666657415e-01};
  FixedPoint P01{-3.25565818622400915405e-01};
  FixedPoint P02{2.01212532134862925881e-01};
  FixedPoint P03{-4.00555345006794114027e-02};
  FixedPoint P04{7.91534994289814532176e-04};
  FixedPoint P05{3.47933107596021167570e-05};
  FixedPoint Q01{-2.40339491173441421878e+00};
  FixedPoint Q02{2.02094576023350569471e+00};
  FixedPoint Q03{-6.88283971605453293030e-01};
  FixedPoint Q04{7.70381505559019352791e-02};
  FixedPoint c;
  if (x < 0.5)
  {
    FixedPoint t = x * x;
    FixedPoint P = t * (P00 + t * (P01 + t * (P02 + t * (P03 + t * (P04 + t * P05)))));
    FixedPoint Q = _1 + t * (Q01 + t * (Q02 + t * (Q03 + t * Q04)));
    FixedPoint R = P / Q;
    return x + x * R;
  }
  else
  {
    FixedPoint w = _1 - x;
    FixedPoint t = w * 0.5;
    FixedPoint P = t * (P00 + t * (P01 + t * (P02 + t * (P03 + t * (P04 + t * P05)))));
    FixedPoint Q = _1 + t * (Q01 + t * (Q02 + t * (Q03 + t * Q04)));
    FixedPoint s = Sqrt(t);
    FixedPoint R = P / Q;
    if (x < 0.975)
    {
      w = s;
      c = (t - w * w) / (s + w);
      P = s * R * 2.0 + c * 2.0;
      Q = Constants.PI_4 - w * 2.0;
      t = Constants.PI_4 - (P - Q);
      return t;
    }
    else
    {
      w = P / Q;
      t = Constants.PI_2 - ((s + s * R) * 2.0);
      return t;
    }
  }
}

/**
 * Calculate the FixedPoint arccos of x acos(x).
 *
 * Special cases
 * * x is NaN    -> asin(x) = NaN
 * * x is +/-inf -> asin(x) = NaN
 * * |x| > 1     -> asin(x) = Nan
 *
 * We use the identity acos(x) = Pi/2 - asin(x) to calculate the value
 *
 * errors for x ∈ [-1, 1):
 * FixedPoint<16,16>: average: 1.94115e-05, max: 0.000305612
 * FixedPoint<32,32>: average: 2.65666e-10, max: 1.78974e-09
 *
 * @param the given x
 * @return the result of acos(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ACos(FixedPoint<I, F> const &x)
{
  if (Abs(x) > _1)
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }

  return Constants.PI_2 - ASin(x);
}

/**
 * Calculate the FixedPoint arctan of x atan(x).
 *
 * Special cases
 * * x is NaN    -> atan(x) = NaN
 * * x is +/-inf -> atan(x) = +/- Pi/2
 * * x < 0       -> atan(x) = -atan(-x)
 * * x > 1       -> atan(x) = Pi/2 - Atan(1/x)
 *
 * The atan(x) is calculated using a Pade approximant, 10th order,
 *
 *            x * (1 + x^2 * (116/57 + x^2 * (2198/1615 + x^2 * (44/133 + x^2 * 5597/264537)))
 * tan(r) = --------------------------------------------------
 *            1 + x^2 * (45/19 + x^2 * (630/323 + x^2 * (210/323 + x^2 * (315/4199 + x^2 *
 63/46189))))
 *
 * errors for x ∈ [-5, 5):
 * FixedPoint<16,16>: average: 9.41805e-06, max: 3.11978e-05
 * FixedPoint<32,32>: average: 9.69576e-10, max: 2.84322e-08

 * @param the given x
 * @return the result of atan(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ATan(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(x))
  {
    return Constants.PI_2;
  }
  else if (isNegInfinity(x))
  {
    return -Constants.PI_2;
  }
  else if (x < _0)
  {
    return -ATan(-x);
  }
  else if (x > _1)
  {
    return Constants.PI_2 - ATan(_1 / x);
  }

  FixedPoint P03 = FixedPoint{116.0 / 57.0};
  FixedPoint P05 = FixedPoint{2198.0 / 1615.0};
  FixedPoint P07 = FixedPoint{44.0 / 133.0};
  FixedPoint P09 = FixedPoint{5597.0 / 264537.0};
  FixedPoint Q02 = FixedPoint{45.0 / 19.0};
  FixedPoint Q04 = FixedPoint{630.0 / 323.0};
  FixedPoint Q06 = FixedPoint{210.0 / 323.0};
  FixedPoint Q08 = FixedPoint{315.0 / 4199.0};
  FixedPoint Q10 = FixedPoint{63.0 / 46189.0};
  FixedPoint x2  = x * x;
  FixedPoint P   = x * (_1 + x2 * (P03 + x2 * (P05 + x2 * (P07 + x2 * P09))));
  FixedPoint Q   = _1 + x2 * (Q02 + x2 * (Q04 + x2 * (Q06 + x2 * (Q08 + x2 * Q10))));

  return std::move(P / Q);
}

/**
 * Calculate the FixedPoint version of atan2(y, x).
 *
 * Special cases
 * * x is NaN    -> atan2(y, x) = NaN
 * * y is NaN    -> atan2(y, x) = NaN
 * * x is +/-inf -> atan(x) = +/- Pi/2
 * * x < 0       -> atan(x) = -atan(-x)
 * * x > 1       -> atan(x) = Pi/2 - Atan(1/x)
 *
 * The atan(x) is calculated using a Pade approximant, 10th order,
 *
 *            x * (1 + x^2 * (116/57 + x^2 * (2198/1615 + x^2 * (44/133 + x^2 * 5597/264537)))
 * tan(r) = --------------------------------------------------
 *            1 + x^2 * (45/19 + x^2 * (630/323 + x^2 * (210/323 + x^2 * (315/4199 + x^2 *
 * 63/46189))))
 *
 * errors for x ∈ (-2, 2), y ∈ (-2, 2):
 * FixedPoint<16,16>: average: 9.10062e-06, max: 2.69345e-05
 * FixedPoint<32,32>: average: 1.81937e-09, max: 2.83877e-08
 *
 * @param the given y
 * @param the given x
 * @return the result of atan2(y, x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ATan2(FixedPoint<I, F> const &y,
                                                   FixedPoint<I, F> const &x)
{
  if (isNaN(y) || isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(y))
  {
    if (isPosInfinity(x))
    {
      return Constants.PI_4;
    }
    else if (isNegInfinity(x))
    {
      return Constants.PI_4 * 3;
    }
    else
    {
      return Constants.PI_2;
    }
  }
  else if (isNegInfinity(y))
  {
    if (isPosInfinity(x))
    {
      return -Constants.PI_4;
    }
    else if (isNegInfinity(x))
    {
      return -Constants.PI_4 * 3;
    }
    else
    {
      return -Constants.PI_2;
    }
  }
  else if (isPosInfinity(x))
  {
    return _0;
  }
  else if (isNegInfinity(x))
  {
    return Sign(y) * Constants.PI;
  }

  if (y < _0)
  {
    return -ATan2(-y, x);
  }

  if (x == _0)
  {
    return Sign(y) * Constants.PI_2;
  }

  FixedPoint u    = y / Abs(x);
  FixedPoint atan = ATan(u);
  if (x < _0)
  {
    return Constants.PI - atan;
  }
  else
  {
    return atan;
  }
}

/**
 * Calculate the FixedPoint hyperbolic sinus of x sinh(x).
 *
 * Special cases
 * * x is NaN    -> sinh(x) = NaN
 * * x is +/-inf -> sinh(x) = +/-inf
 * Calculated using the definition formula:
 *
 *            e^x - e^(-x)
 * sinh(x) = --------------
 *                2
 *
 * errors for x ∈ [-3, 3):
 * FixedPoint<16,16>: average: 6.63577e-05, max: 0.000479903
 * errors for x ∈ [-5, 5):
 * FixedPoint<32,32>: average: 7.39076e-09, max: 7.90546e-08

 * @param the given x
 * @return the result of sinh(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::SinH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }
  else if (isNegInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.NEGATIVE_INFINITY;
  }

  FixedPoint half{0.5};
  return std::move(half * (Exp(x) - Exp(-x)));
}

/**
 * Calculate the FixedPoint hyperbolic cosinus of x cosh(x).
 *
 * Special cases
 * * x is NaN    -> cosh(x) = NaN
 * * x is +/-inf -> cosh(x) = +inf
 * Calculated using the definition formula:
 *
 *            e^x + e^(-x)
 * sinh(x) = --------------
 *                2
 *
 * errors for x ∈ [-3, 3):
 * FixedPoint<16,16>: average: 6.92127e-05, max: 0.000487532
 * errors for x ∈ [-5, 5):
 * FixedPoint<32,32>: average: 7.30786e-09, max: 7.89509e-08

 * @param the given x
 * @return the result of cosh(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::CosH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }

  FixedPoint half{0.5};
  return std::move(half * (Exp(x) + Exp(-x)));
}

/**
 * Calculate the FixedPoint hyperbolic tangent of x, tanh(x).
 *
 * Special cases
 * * x is NaN    -> tanh(x) = NaN
 * * x is +/-inf -> tanh(x) = +/-1
 * Calculated using the definition formula:
 *
 *            e^x - e^(-x)
 * sinh(x) = --------------
 *            e^x + e^(-x)
 *
 * errors for x ∈ [-3, 3):
 * FixedPoint<16,16>: average: 1.25046e-05, max: 7.0897e-05
 * FixedPoint<32,32>: average: 1.7648e-10,  max: 1.19186e-09

 * @param the given x
 * @return the result of tanh(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::TanH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }
  else if (isNegInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.NEGATIVE_INFINITY;
  }

  FixedPoint e1     = Exp(x);
  FixedPoint e2     = Exp(-x);
  FixedPoint result = (e1 - e2) / (e1 + e2);
  return std::move(result);
}

/**
 * Calculate the FixedPoint inverse of hyperbolic sinus of x, asinh(x).
 *
 * Special cases
 * * x is NaN    -> asinh(x) = NaN
 * * x is +/-inf -> asinh(x) = +/-inf
 * Calculated using the  formula:
 *
 * asinh(x) = Log(x + Sqrt(x^2 + 1))
 *
 * errors for x ∈ [-3, 3):
 * FixedPoint<16,16>: average: 5.59257e-05, max: 0.00063489
 * FixedPoint<32,32>: average: 3.49254e-09, max: 2.62839e-08

 * @param the given x
 * @return the result of asinh(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ASinH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }
  else if (isNegInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.NEGATIVE_INFINITY;
  }

  return Log(x + Sqrt(x * x + _1));
}

/**
 * Calculate the FixedPoint inverse of hyperbolic cosinus of x, acosh(x).
 *
 * Special cases
 * * x is NaN    -> acosh(x) = NaN
 * * x is +inf   -> acosh(x) = +inf
 * * x < 1       -> acosh(x) = NaN
 * Calculated using the definition formula:
 *
 * acosh(x) = Log(x + Sqrt(x^2 - 1))
 *
 * errors for x ∈ [1, 3):
 * FixedPoint<16,16>: average: 8.53834e-06, max: 6.62567e-05
* errors for x ∈ [1, 5):
 * FixedPoint<32,32>: average: 2.37609e-09, max: 2.28507e-08

 * @param the given x
 * @return the result of acosh(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ACosH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return Constants.POSITIVE_INFINITY;
  }
  else if (isNegInfinity(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }

  // ACosH(x) is defined in the [1, +inf) range
  if (x < _1)
  {
    return Constants.NaN;
  }
  return Log(x + Sqrt(x * x - _1));
}

/**
 * Calculate the FixedPoint inverse of hyperbolic tangent of x, atanh(x).
 *
 * Special cases
 * * x is NaN    -> atanh(x) = NaN
 * * x is +/-inf -> atanh(x) = NaN
 * Calculated using the definition formula:
 *
 *            1        1 + x
 * atanh(x) = - * Log(-------)
 *            2        1 - x
 *
 * errors for x ∈ (-1, 1):
 * FixedPoint<16,16>: average: 2.08502e-05, max: 0.000954267
 * FixedPoint<32,32>: average: 1.47673e-09, max: 1.98984e-07

 * @param the given x
 * @return the result of atanh(x)
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ATanH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }
  else if (isInfinity(x))
  {
    fp_state |= STATE_NAN;
    return Constants.NaN;
  }

  // ATanH(x) is defined in the (-1, 1) range
  if (Abs(x) > _1)
  {
    return Constants.NaN;
  }
  FixedPoint half{0.5};
  return half * Log((_1 + x) / (_1 - x));
}

}  // namespace fixed_point
}  // namespace fetch
