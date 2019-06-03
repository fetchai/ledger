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

#include "core/assert.hpp"
#include "meta/tags.hpp"
#include "meta/type_traits.hpp"
#include "vectorise/platform.hpp"

#include <functional>
#include <iostream>
#include <limits>
#include <sstream>

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

  return static_cast<int32_t>((sizeof(uint64_t) * 8)) - platform::CountLeadingZeroes64(n);
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
  static FixedPoint const CONST_ZERO; /* 0 */
  static FixedPoint const CONST_ONE;  /* 1 */
  static FixedPoint const CONST_SMALLEST_FRACTION;
  static FixedPoint const CONST_E;            /* e */
  static FixedPoint const CONST_LOG2E;        /* log_2 e */
  static FixedPoint const CONST_LOG210;       /* log_2 10 */
  static FixedPoint const CONST_LOG10E;       /* log_10 e */
  static FixedPoint const CONST_LN2;          /* log_e 2 */
  static FixedPoint const CONST_LN10;         /* log_e 10 */
  static FixedPoint const CONST_PI;           /* pi */
  static FixedPoint const CONST_PI_2;         /* pi/2 */
  static FixedPoint const CONST_PI_4;         /* pi/4 */
  static FixedPoint const CONST_INV_PI;       /* 1/pi */
  static FixedPoint const CONST_2_INV_PI;     /* 2/pi */
  static FixedPoint const CONST_2_INV_SQRTPI; /* 2/sqrt(pi) */
  static FixedPoint const CONST_SQRT2;        /* sqrt(2) */
  static FixedPoint const CONST_INV_SQRT2;    /* 1/sqrt(2) */
  static FixedPoint const MAX_EXP;
  static FixedPoint const MIN_EXP;
  static FixedPoint const CONST_MAX;
  static FixedPoint const CONST_MIN;
  static FixedPoint const NaN;
  static FixedPoint const POSITIVE_INFINITY;
  static FixedPoint const NEGATIVE_INFINITY;

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
  constexpr FixedPoint &operator=(T const &n);

  ///////////////////////////////////////////////////
  /// comparison operators for FixedPoint objects ///
  ///////////////////////////////////////////////////

  constexpr bool operator==(FixedPoint const &o) const;
  constexpr bool operator!=(FixedPoint const &o) const;
  constexpr bool operator<(FixedPoint const &o) const;
  constexpr bool operator>(FixedPoint const &o) const;

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

  //////////////////////
  /// math operators ///
  //////////////////////

  constexpr FixedPoint operator+(FixedPoint const &n) const;
  template <typename T>
  constexpr FixedPoint operator+(T const &n) const;
  constexpr FixedPoint operator-(FixedPoint const &n) const;
  template <typename T>
  constexpr FixedPoint operator-(T const &n) const;
  constexpr FixedPoint operator*(FixedPoint const &n) const;
  template <typename T>
  constexpr FixedPoint operator*(T const &n) const;
  constexpr FixedPoint operator/(FixedPoint const &n) const;
  template <typename T>
  constexpr FixedPoint  operator/(T const &n) const;
  constexpr FixedPoint &operator+=(FixedPoint const &n);
  constexpr FixedPoint &operator-=(FixedPoint const &n);
  constexpr FixedPoint &operator&=(FixedPoint const &n);
  constexpr FixedPoint &operator|=(FixedPoint const &n);
  constexpr FixedPoint &operator^=(FixedPoint const &n);
  constexpr FixedPoint &operator*=(FixedPoint const &n);
  constexpr FixedPoint &operator/=(FixedPoint const &n);
  constexpr FixedPoint &operator>>=(FixedPoint const &n);
  constexpr FixedPoint &operator>>=(const int &n);
  constexpr FixedPoint &operator<<=(FixedPoint const &n);
  constexpr FixedPoint &operator<<=(const int &n);

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
  static constexpr FixedPoint Remainder(FixedPoint const &x, FixedPoint const &y);
  static constexpr FixedPoint Fmod(FixedPoint const &x, FixedPoint const &y);
  static constexpr FixedPoint Abs(FixedPoint const &x);
  static constexpr FixedPoint Sign(FixedPoint const &x);
  static constexpr bool       isNaN(FixedPoint const &x);

private:
  Type data_{0};  // the value to be stored

  static std::function<FixedPoint(FixedPoint const &x)> SinPi2QuadrantFuncs[4];
  static std::function<FixedPoint(FixedPoint const &x)> CosPi2QuadrantFuncs[4];

  // this makes it simpler to create a fixed point object from
  // a native type without scaling
  // use "FixedPoint::from_base" in order to perform this.
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
    int        k = 0;
    FixedPoint four{4};
    while (x > four)
    {
      k++;
      x >>= 2;
    }
    if (x < CONST_ONE)
    {
      while (x < CONST_ONE)
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
    assert(r <= CONST_PI_2);

    if (r > CONST_PI_4)
    {
      return std::move(CosPi2(r - CONST_PI_2));
    }

    return std::move(SinApproxPi4(r));
  }

  static constexpr FixedPoint CosPi2(FixedPoint const &r)
  {
    assert(r <= CONST_PI_2);

    if (r > CONST_PI_4)
    {
      return std::move(SinPi2(CONST_PI_2 - r));
    }

    return std::move(CosApproxPi4(r));
  }

  static constexpr FixedPoint<16, 16> SinApproxPi4(FixedPoint<16, 16> const &r)
  {
    assert(r <= CONST_PI_4);

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
    assert(r <= CONST_PI_4);

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
    assert(r <= CONST_PI_4);

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
FixedPoint<I, F> const FixedPoint<I, F>::TOLERANCE(
    0, FixedPoint<I, F>::BaseTypeInfo::tolerance); /* 0 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_ZERO{0}; /* 0 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_ONE{1}; /* 1 */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_SMALLEST_FRACTION(0, FixedPoint::SMALLEST_FRACTION);
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_E{2.718281828459045235360287471352662498}; /* e */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_LOG2E{
    1.442695040888963407359924681001892137}; /* log_2 e */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_LOG210{3.3219280948874}; /* log_2 10 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_LOG10E{
    0.434294481903251827651128918916605082}; /* log_10 e */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_LN2{
    0.693147180559945309417232121458176568}; /* log_e 2 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_LN10{
    2.302585092994045684017991454684364208}; /* log_e 10 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_PI{3.141592653589793238462643383279502884}; /* pi */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_PI_2{
    1.570796326794896619231321691639751442}; /* pi/2 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_PI_4{
    0.785398163397448309615660845819875721}; /* pi/4 */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_INV_PI{
    0.318309886183790671537767526745028724}; /* 1/pi */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_2_INV_PI{
    0.636619772367581343075535053490057448}; /* 2/pi */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_2_INV_SQRTPI{
    1.128379167095512573896158903121545172}; /* 2/sqrt(pi) */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_SQRT2{
    1.414213562373095048801688724209698079}; /* sqrt(2) */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_INV_SQRT2{
    0.707106781186547524400844362104849039}; /* 1/sqrt(2) */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::MAX_EXP =
    FixedPoint::FromBase(FixedPoint<I, F>::BaseTypeInfo::max_exp); /* maximum exponent for Exp() */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::MIN_EXP =
    -FixedPoint<I, F>::MAX_EXP; /* minimum exponent for Exp() */
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_MAX{FixedPoint::FromBase(FixedPoint::MAX)};
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_MIN{FixedPoint::FromBase(FixedPoint::MIN)};
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::NaN{
    FixedPoint::FromBase(FixedPoint::Type(1) << (FixedPoint::TOTAL_BITS - 1))};

template <std::uint16_t I, std::uint16_t F>
std::ostream &operator<<(std::ostream &s, FixedPoint<I, F> const &n)
{
  std::ios_base::fmtflags f(s.flags());
  s << std::setprecision(F / 4);
  s << std::fixed;
  if (!FixedPoint<I, F>::isNaN(n))
  {
    s << double(n);
  }
  else
  {
    s << "NaN";
  }
  s << " (0x" << std::hex << n.Data() << ")";
  s.flags(f);
  return s;
}

////////////////////
/// constructors ///
////////////////////

/**
 * Templated constructor existing only for T is an integer and assigns data
 * @tparam T any integer type
 * @param n integer value to set FixedPoint to
 */
template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F>::FixedPoint(T n, meta::IfIsInteger<T> *)
  : data_{static_cast<typename FixedPoint<I, F>::Type>(n)}
{
  assert(CheckNoOverflow(n));
  Type s    = (data_ < 0) - (data_ > 0);
  Type abs_ = s * data_;
  abs_ <<= FRACTIONAL_BITS;
  data_ = s * abs_;
}

template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F>::FixedPoint(T n, meta::IfIsFloat<T> *)
  : data_(static_cast<typename FixedPoint<I, F>::Type>(n * ONE_MASK))
{
  assert(CheckNoOverflow(n * ONE_MASK));
  //    assert(CheckNoRounding<T>());
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F>::FixedPoint(FixedPoint<I, F> const &o)
  : data_{o.data_}
{}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F>::FixedPoint(const typename FixedPoint<I, F>::Type &        integer,
                                       const typename FixedPoint<I, F>::UnsignedType &fraction)
  : data_{(INTEGER_MASK & (Type(integer) << FRACTIONAL_BITS)) | Type(fraction & FRACTIONAL_MASK)}
{}

///////////////////
/// conversions ///
///////////////////

template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::Integer() const
{
  if (isNaN(*this))
  {
    throw std::overflow_error("Cannot get the integer part of a NaN value!");
  }
  return Type((data_ & INTEGER_MASK) >> FRACTIONAL_BITS);
}

template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::Fraction() const
{
  if (isNaN(*this))
  {
    throw std::overflow_error("Cannot get the fraction part of a NaN value!");
  }
  return (data_ & FRACTIONAL_MASK);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Floor(FixedPoint<I, F> const &o)
{
  if (isNaN(o))
  {
    return NaN;
  }
  return std::move(FixedPoint{o.Integer()});
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Round(FixedPoint<I, F> const &o)
{
  if (isNaN(o))
  {
    return NaN;
  }
  return std::move(Floor(o + FixedPoint{0.5}));
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::FromBase(typename FixedPoint<I, F>::Type n)
{
  return FixedPoint(n, NoScale());
}

/////////////////////////
/// casting operators ///
/////////////////////////

template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F>::operator double() const
{
  return (static_cast<double>(data_) / ONE_MASK);
}

template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F>::operator float() const
{
  return (static_cast<float>(data_) / ONE_MASK);
}

template <std::uint16_t I, std::uint16_t F>
template <typename T>
FixedPoint<I, F>::operator T() const
{
  return (static_cast<T>(Integer()));
}

/////////////////
/// operators ///
/////////////////

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator=(FixedPoint<I, F> const &o)
{
  if (isNaN(o))
  {
    throw std::overflow_error("Cannot assing NaN value!");
  }
  data_ = o.data_;
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator=(T const &n)
{
  data_ = {static_cast<Type>(n) << static_cast<Type>(FRACTIONAL_BITS)};
  assert(CheckNoOverflow(n));
  return *this;
}

///////////////////////////////////////////////////
/// comparison operators for FixedPoint objects ///
///////////////////////////////////////////////////

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

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator<(FixedPoint const &o) const
{
  if (isNaN(*this) || isNaN(o))
  {
    return false;
  }
  else
  {
    return (data_ < o.Data());
  }
}

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator>(FixedPoint const &o) const
{
  if (isNaN(*this) || isNaN(o))
  {
    return false;
  }
  else
  {
    return (data_ > o.Data());
  }
}

////////////////////////////////////////////////
/// comparison operators against other types ///
////////////////////////////////////////////////

template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator==(OtherType const &o) const
{
  return (data_ == FixedPoint(o).Data());
}

template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator!=(OtherType const &o) const
{
  return (data_ != FixedPoint(o).Data());
}

template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator<(OtherType const &o) const
{
  return (data_ < FixedPoint(o).Data());
}

template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator>(OtherType const &o) const
{
  return (data_ > FixedPoint(o).Data());
}

template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator<=(OtherType const &o) const
{
  return (data_ <= FixedPoint(o).Data());
}

template <std::uint16_t I, std::uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator>=(OtherType const &o) const
{
  return (data_ >= FixedPoint(o).Data());
}

///////////////////////
/// unary operators ///
///////////////////////

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::operator!() const
{
  return !data_;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator~() const
{
  FixedPoint t(*this);
  t.data_ = ~t.data_;
  return t;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator-() const
{
  FixedPoint t(*this);
  t.data_ = -t.data_;
  return t;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator++()
{
  assert(CheckNoOverflow(data_ + CONST_ONE.Data()));
  data_ += ONE_MASK;
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator--()
{
  assert(CheckNoOverflow(data_ - CONST_ONE.Data()));
  data_ -= ONE_MASK;
  return *this;
}

//////////////////////
/// math operators ///
//////////////////////

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator+(FixedPoint<I, F> const &n) const
{
  assert(CheckNoOverflow(data_ + n.Data()));
  Type fp = data_ + n.Data();
  return FromBase(fp);
}

template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator+(T const &n) const
{
  return FixedPoint(T(data_) + n);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator-(FixedPoint<I, F> const &n) const
{
  assert(CheckNoOverflow(data_ - n.Data()));
  Type fp = data_ - n.Data();
  return FromBase(fp);
}

template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator-(T const &n) const
{
  return FixedPoint(T(data_) - n);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator*(FixedPoint<I, F> const &n) const
{
  if (isNaN(n))
  {
    return NaN;
  }
  NextType prod = NextType(data_) * NextType(n.Data());
  assert(CheckNoOverflow(Type(prod >> FRACTIONAL_BITS)));
  Type fp = Type(prod >> FRACTIONAL_BITS);
  return FromBase(fp);
}

template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator*(T const &n) const
{
  return *this * FixedPoint(n);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator/(FixedPoint<I, F> const &n) const
{
  if (n == CONST_ZERO)
  {
    throw std::overflow_error("Division by zero!");
  }
  FixedPoint sign      = Sign(*this);
  FixedPoint abs_n     = Abs(*this);
  NextType   numerator = NextType(abs_n.Data()) << FRACTIONAL_BITS;
  NextType   quotient  = numerator / NextType(n.Data());
  return sign * FromBase(Type(quotient));
}

template <std::uint16_t I, std::uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator/(T const &n) const
{
  return *this / FixedPoint(n);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator+=(FixedPoint<I, F> const &n)
{
  assert(CheckNoOverflow(data_ + n.Data()));
  data_ += n.Data();
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator-=(FixedPoint<I, F> const &n)
{
  assert(CheckNoOverflow(data_ - n.Data()));
  data_ -= n.Data();
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator&=(FixedPoint<I, F> const &n)
{
  assert(CheckNoOverflow(data_ & n.Data()));
  data_ &= n.Data();
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator|=(FixedPoint<I, F> const &n)
{
  assert(CheckNoOverflow(data_ | n.Data()));
  data_ |= n.Data();
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator^=(FixedPoint<I, F> const &n)
{
  assert(CheckNoOverflow(data_ ^ n.Data()));
  data_ ^= n.Data();
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator*=(FixedPoint<I, F> const &n)
{
  Multiply(*this, n, *this);
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator/=(FixedPoint<I, F> const &n)
{
  FixedPoint temp;
  *this = Divide(*this, n, temp);
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator>>=(FixedPoint<I, F> const &n)
{
  data_ >>= n.Integer();
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator>>=(const int &n)
{
  data_ >>= n;
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator<<=(FixedPoint<I, F> const &n)
{
  data_ <<= n.Integer();
  return *this;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator<<=(const int &n)
{
  data_ <<= n;
  return *this;
}

////////////
/// swap ///
////////////

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

template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::Data() const
{
  return data_;
}

template <std::uint16_t I, std::uint16_t F>
constexpr void FixedPoint<I, F>::SetData(typename FixedPoint<I, F>::Type const n) const
{
  data_ = n;
}

///////////////////////////////////////////////////////////////////
/// FixedPoint implementations of common mathematical functions ///
///////////////////////////////////////////////////////////////////

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Exp(FixedPoint<I, F> const &x)
{
  if (x < MIN_EXP)
  {
    return CONST_ZERO;
  }
  else if (x > MAX_EXP)
  {
    throw std::overflow_error("Exp() does not support exponents larger than MAX_EXP");
  }
  else if (x == CONST_ONE)
  {
    return CONST_E;
  }
  else if (x == CONST_ZERO)
  {
    return CONST_ONE;
  }
  else if (x < CONST_ZERO)
  {
    return CONST_ONE / Exp(-x);
  }

  // Find integer k and r ∈ [-0.5, 0.5) such as: x = k*ln2 + r
  // Then exp(x) = 2^k * e^r
  FixedPoint k = x / CONST_LN2;
  k            = Floor(k);

  FixedPoint r = x - k * CONST_LN2;
  FixedPoint e1{CONST_ONE};
  e1 <<= k;

  // We take the Pade 4,4 approximant for the exp(x) function:
  // https://en.wikipedia.org/wiki/Pad%C3%A9_table
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
  FixedPoint P  = CONST_ONE + r + r2 + r3 + r4 + r5;
  FixedPoint Q  = CONST_ONE - r + r2 - r3 + r4 - r5;
  FixedPoint e2 = P / Q;

  return std::move(e1 * e2);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Log2(FixedPoint<I, F> const &x)
{
  if (x == CONST_ONE)
  {
    return CONST_ZERO;
  }
  else if (x == CONST_ZERO)
  {
    return CONST_ONE;
  }
  else if (x == CONST_SMALLEST_FRACTION)
  {
    return FixedPoint{-FRACTIONAL_BITS};
  }
  else if (isNaN(x))
  {
    return NaN;
  }
  else if (x < CONST_ZERO)
  {
    return NaN;
  }

  /* Argument Reduction: find k and f such that
      x = 2^k * f,
      We can get k easily with HighestSetBit(), but we have to subtract the fractional bits to
     mark negative logarithms for numbers < 1.
  */
  bool       adjustment = false;
  FixedPoint y          = x;
  if (y < 1.0)
  {
    y          = CONST_ONE / x;
    adjustment = true;
  }
  Type       k = HighestSetBit(y.Data()) - Type(FRACTIONAL_BITS);
  FixedPoint k_shifted{Type(1) << k};
  FixedPoint f = y / k_shifted;

  FixedPoint P00{137};
  FixedPoint P01{1762};  // P03 also
  FixedPoint P02{3762};
  FixedPoint P04{137};
  FixedPoint Q0{30};
  FixedPoint Q01{24};  // Q03 also
  FixedPoint Q02{76};
  FixedPoint P = (-CONST_ONE + f) * (P00 + f * (P01 + f * (P02 + f * (P01 + f * P04))));
  FixedPoint Q =
      Q0 * (CONST_ONE + f) * (CONST_ONE + f * (Q01 + f * (Q02 + f * (Q01 + f)))) * CONST_LN2;
  FixedPoint R = P / Q;

  if (adjustment)
  {
    return std::move(-FixedPoint(k) - R);
  }
  else
  {
    return std::move(FixedPoint(k) + R);
  }
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Log(FixedPoint<I, F> const &x)
{
  return Log2(x) / CONST_LOG2E;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Log10(FixedPoint<I, F> const &x)
{
  return Log2(x) / CONST_LOG210;
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Sqrt(FixedPoint<I, F> const &x)
{
  if (x == CONST_ONE)
  {
    return CONST_ONE;
  }
  if (x == CONST_ZERO)
  {
    return CONST_ZERO;
  }
  if (x < CONST_ZERO)
  {
    return NaN;
  }

  FixedPoint x0 = x;
  int        k  = ReduceSqrt(x0);

  if (x0 != CONST_ONE)
  {
    // Do a Pade Approximation, 4th order around 1.
    FixedPoint P01{3};
    FixedPoint P02{11};
    FixedPoint P03{9};
    FixedPoint Q01{3};
    FixedPoint Q02{27};
    FixedPoint Q03{33};
    FixedPoint P = (CONST_ONE + P01 * x0) * (CONST_ONE + P01 * x0 * (P02 + x0 * (P03 + x0)));
    FixedPoint Q = (Q01 + x0) * (Q01 + x0 * (Q02 + x0 * (Q03 + x0)));
    FixedPoint R = P / Q;

    // Tune the approximation with 2 iterations of Goldsmith's algorithm (converges faster than
    // NR)
    FixedPoint half{0.5};
    FixedPoint y_n = CONST_ONE / R;
    FixedPoint x_n = x0 * y_n;
    FixedPoint h_n = half * y_n;
    FixedPoint r_n;
    r_n = half - x_n * h_n;
    x_n = x_n + x_n * r_n;
    h_n = h_n + h_n * r_n;
    r_n = half - x_n * h_n;
    x_n = x_n + x_n * r_n;
    h_n = h_n + h_n * r_n;

    // Final result should be accurate within ~1e-7.
    x0 = x_n;
  }

  FixedPoint twok{CONST_ONE};
  if (k < 0)
  {
    twok >>= -k;
  }
  else
  {
    twok <<= k;
  }

  return std::move(twok * x0);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Pow(FixedPoint<I, F> const &x,
                                                 FixedPoint<I, F> const &y)
{
  if (x == CONST_ZERO)
  {
    if (y == CONST_ZERO)
    {
      throw std::runtime_error("Pow(0, 0): 0^0 mathematical operation not defined!");
    }
    else
    {
      return CONST_ZERO;
    }
  }

  if (y == CONST_ZERO)
  {
    return CONST_ONE;
  }

  if (x < CONST_ZERO)
  {
    if (y.Fraction() != 0)
    {
      throw std::runtime_error(
          "Pow(x, y): x^y where x < 0 and y non-integer: mathematical operation not defined!");
    }
    else
    {
      FixedPoint pow{x};
      FixedPoint t{Abs(y)};
      while (--t)
      {
        pow *= x;
      }
      if (y > 0)
      {
        return pow;
      }
      else
      {
        return CONST_ONE / pow;
      }
    }
  }
  FixedPoint s   = CONST_ONE * ((y.Integer() + 1) & 1) + Sign(x) * (y.Integer() & 1);
  FixedPoint pow = s * Exp(y * Log(Abs(x)));
  return std::move(pow);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Sin(FixedPoint<I, F> const &x)
{
  if (x < CONST_ZERO)
  {
    return -Sin(-x);
  }

  FixedPoint r = Fmod(x, CONST_PI * 2);

  if (r == CONST_ZERO)
  {
    return CONST_ZERO;
  }

  FixedPoint quadrant = Floor(r / CONST_PI_2);

  return SinPi2QuadrantFuncs[(size_t)quadrant](r - CONST_PI_2 * quadrant);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Cos(FixedPoint<I, F> const &x)
{
  FixedPoint r = Fmod(Abs(x), CONST_PI * 2);
  if (r == CONST_ZERO)
  {
    return CONST_ONE;
  }

  FixedPoint quadrant = Floor(r / CONST_PI_2);

  return CosPi2QuadrantFuncs[(size_t)quadrant](r - CONST_PI_2 * quadrant);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Tan(FixedPoint<I, F> const &x)
{
  if (x < CONST_ZERO)
  {
    return -Tan(-x);
  }

  if (x == CONST_PI_2)
  {
    // (843, private) Replace with POSITIVE_INFINITY
    return NaN;
  }

  FixedPoint r = Fmod(x, CONST_PI);
  FixedPoint P01{-0.1212121212121212};    // 4/33
  FixedPoint P02{0.00202020202020202};    // 1/495
  FixedPoint Q01{-0.4545454545454545};    // 5/11
  FixedPoint Q02{0.0202020202020202};     // 2/99
  FixedPoint Q03{-9.62000962000962e-05};  // 1/10395
  if (r <= CONST_PI_4)
  {
    FixedPoint r2 = r * r;
    FixedPoint P  = r * (CONST_ONE + r2 * (P01 + r2 * P02));
    FixedPoint Q  = CONST_ONE + r2 * (Q01 + r2 * (Q02 + r2 * Q03));
    return std::move(P / Q);
  }
  else if (r < CONST_PI_2)
  {
    FixedPoint y  = r - CONST_PI_2;
    FixedPoint y2 = y * y;
    FixedPoint P  = -(CONST_ONE + y2 * (Q01 + y2 * (Q02 + y2 * Q03)));
    FixedPoint Q  = -CONST_PI_2 + r + y2 * y * (P01 + y2 * P02);
    return std::move(P / Q);
  }
  else
  {
    return Tan(r - CONST_PI);
  }
}

/* Based on the NetBSD implementation
 *  Since  asin(x) = x + x^3/6 + x^5*3/40 + x^7*15/336 + ...
 *  we approximate asin(x) on [0,0.5] by
 *    asin(x) = x + x*x^2*R(x^2)
 *  where
 *    R(x^2) is a rational approximation of (asin(x)-x)/x^3
 *  and its remez error is bounded by
 *    |(asin(x)-x)/x^3 - R(x^2)| < 2^(-58.75)
 *
 *  For x in [0.5,1]
 *    asin(x) = pi/2-2*asin(sqrt((1-x)/2))
 *  Let y = (1-x), z = y/2, s := sqrt(z), and pio2_hi+pio2_lo=pi/2;
 *  then for x>0.98
 *    asin(x) = pi/2 - 2*(s+s*z*R(z))
 *      = pio2_hi - (2*(s+s*z*R(z)) - pio2_lo)
 *  For x<=0.98, let pio4_hi = pio2_hi/2, then
 *    f = hi part of s;
 *    c = sqrt(z) - f = (z-f*f)/(s+f)   ...f+c=sqrt(z)
 *  and
 *    asin(x) = pi/2 - 2*(s+s*z*R(z))
 *      = pio4_hi+(pio4-2s)-(2s*z*R(z)-pio2_lo)
 *      = pio4_hi+(pio4-2f)-(2s*z*R(z)-(pio2_lo+2c))
 *
 * Special cases:
 *  if x is NaN, return x itself;
 *  if |x|>1, return NaN with invalid signal.
 */
template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ASin(FixedPoint<I, F> const &x)
{
  if (x < 0)
  {
    return -ASin(-x);
  }

  if (x > CONST_ONE)
  {
    return NaN;
  }

  FixedPoint p0{1.66666666666666657415e-01}, p1{-3.25565818622400915405e-01},
      p2{2.01212532134862925881e-01}, p3{-4.00555345006794114027e-02},
      p4{7.91534994289814532176e-04}, p5{3.47933107596021167570e-05},
      q1{-2.40339491173441421878e+00}, q2{2.02094576023350569471e+00},
      q3{-6.88283971605453293030e-01}, q4{7.70381505559019352791e-02},
      pio2_hi{1.57079632679489655800e+00}, pio4_hi{7.85398163397448278999e-01};
  FixedPoint c;
  if (x < 0.5)
  {
    FixedPoint t = x * x;
    FixedPoint P = t * (p0 + t * (p1 + t * (p2 + t * (p3 + t * (p4 + t * p5)))));
    FixedPoint Q = CONST_ONE + t * (q1 + t * (q2 + t * (q3 + t * q4)));
    FixedPoint R = P / Q;
    return x + x * R;
  }
  else
  {
    FixedPoint w = CONST_ONE - x;
    FixedPoint t = w * 0.5;
    FixedPoint P = t * (p0 + t * (p1 + t * (p2 + t * (p3 + t * (p4 + t * p5)))));
    FixedPoint Q = CONST_ONE + t * (q1 + t * (q2 + t * (q3 + t * q4)));
    FixedPoint s = Sqrt(t);
    FixedPoint R = P / Q;
    if (x < 0.975)
    {
      w = s;
      c = (t - w * w) / (s + w);
      P = s * R * 2.0 + c * 2.0;
      Q = pio4_hi - w * 2.0;
      t = pio4_hi - (P - Q);
      return t;
    }
    else
    {
      w = P / Q;
      t = pio2_hi - ((s + s * R) * 2.0);
      return t;
    }
  }
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ACos(FixedPoint<I, F> const &x)
{
  if (Abs(x) > CONST_ONE)
  {
    return NaN;
  }

  return CONST_PI_2 - ASin(x);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ATan(FixedPoint<I, F> const &x)
{
  if (x < 0)
  {
    return -ATan(-x);
  }

  if (x > 1)
  {
    return CONST_PI_2 - ATan(CONST_ONE / x);
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
  FixedPoint x3  = x2 * x;
  FixedPoint x4  = x3 * x;
  FixedPoint x5  = x4 * x;
  FixedPoint x6  = x5 * x;
  FixedPoint x7  = x6 * x;
  FixedPoint x8  = x7 * x;
  FixedPoint x9  = x8 * x;
  FixedPoint x10 = x9 * x;
  FixedPoint P   = x + P03 * x3 + P05 * x5 + P07 * x7 + P09 * x9;
  FixedPoint Q   = CONST_ONE + Q02 * x2 + Q04 * x4 + Q06 * x6 + Q08 * x8 + Q10 * x10;

  return std::move(P / Q);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ATan2(FixedPoint<I, F> const &y,
                                                   FixedPoint<I, F> const &x)
{
  if (isNaN(y) || isNaN(x))
  {
    return NaN;
  }

  if (y < 0)
  {
    return -ATan2(-y, x);
  }

  if (x == 0)
  {
    return Sign(y) * CONST_PI_2;
  }

  FixedPoint u    = y / Abs(x);
  FixedPoint atan = ATan(u);
  if (x < 0)
  {
    return CONST_PI - atan;
  }
  else
  {
    return atan;
  }
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::SinH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    return NaN;
  }

  FixedPoint half{0.5};
  return std::move(half * (Exp(x) - Exp(-x)));
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::CosH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    return NaN;
  }

  FixedPoint half{0.5};
  return std::move(half * (Exp(x) + Exp(-x)));
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::TanH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    return NaN;
  }

  FixedPoint e1     = Exp(x);
  FixedPoint e2     = Exp(-x);
  FixedPoint result = (e1 - e2) / (e1 + e2);
  return std::move(result);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ASinH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    return NaN;
  }

  return Log(x + Sqrt(x * x + CONST_ONE));
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ACosH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    return NaN;
  }

  if (x < CONST_ONE)
  {
    return NaN;
  }
  return Log(x + Sqrt(x * x - CONST_ONE));
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ATanH(FixedPoint<I, F> const &x)
{
  if (isNaN(x))
  {
    return NaN;
  }

  if (x > CONST_ONE)
  {
    return NaN;
  }
  FixedPoint half{0.5};
  return half * Log((CONST_ONE + x) / (CONST_ONE - x));
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Remainder(FixedPoint<I, F> const &x,
                                                       FixedPoint<I, F> const &y)
{
  FixedPoint result = x / y;
  return std::move(x - Round(result) * y);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Fmod(FixedPoint<I, F> const &x,
                                                  FixedPoint<I, F> const &y)
{
  FixedPoint result = Remainder(Abs(x), Abs(y));
  if (result < CONST_ZERO)
  {
    result += Abs(y);
  }
  return std::move(Sign(x) * result);
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Abs(FixedPoint<I, F> const &x)
{
  return std::move(x * Sign(x));
}

template <std::uint16_t I, std::uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Sign(FixedPoint<I, F> const &x)
{
  return std::move(FixedPoint{Type((x >= CONST_ZERO) - (x < CONST_ZERO))});
}

template <std::uint16_t I, std::uint16_t F>
constexpr bool FixedPoint<I, F>::isNaN(FixedPoint<I, F> const &x)
{
  if (x.Data() == NaN.Data())
  {
    return true;
  }
  else
  {
    return false;
  }
}

}  // namespace fixed_point
}  // namespace fetch
