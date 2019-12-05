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
#include "vectorise/uint/int.hpp"
#include "vectorise/uint/uint.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <limits>
#include <ostream>

namespace fetch {
namespace fixed_point {

template <uint16_t I, uint16_t F>
class FixedPoint;

using fp32_t  = FixedPoint<16, 16>;
using fp64_t  = FixedPoint<32, 32>;
using fp128_t = FixedPoint<64, 64>;

// struct for inferring what underlying types to use
template <int T>
struct TypeFromSize
{
  static const bool is_valid = false;  // for template matches specialisation
  using ValueType            = void;
};

#if (__SIZEOF_INT128__ == 16)
// 256 bit implementation
template <>
struct TypeFromSize<256>
{
  static constexpr bool     is_valid = true;
  static constexpr uint16_t size     = 256;
  using ValueType                    = fetch::vectorise::Int<256>;
  using UnsignedType                 = fetch::vectorise::UInt<256>;
  using SignedType                   = fetch::vectorise::Int<256>;
  // using NextSize                   = TypeFromSize<256>;
};
// 128 bit implementation
template <>
struct TypeFromSize<128>
{
  static constexpr bool     is_valid   = true;
  static constexpr uint16_t size       = 128;
  using ValueType                      = int128_t;
  using UnsignedType                   = uint128_t;
  using SignedType                     = int128_t;
  using NextSize                       = TypeFromSize<256>;
  static constexpr uint16_t  decimals  = 18;
  static constexpr ValueType tolerance = 0x100000000000;  // 0,00000095367431640625
  static constexpr ValueType max_exp =
      (static_cast<uint128_t>(0x2b) << 64) | 0xab13e5fca20e0000;  // 43.6682723752765511
  static constexpr UnsignedType min_exp = (static_cast<uint128_t>(0xffffffffffffffd4) << 64) |
                                          0x54ec1a035df20000;  // -43.6682723752765511
};
#endif

// 64 bit implementation
template <>
struct TypeFromSize<64>
{
  static constexpr bool     is_valid      = true;
  static constexpr uint16_t size          = 64;
  using ValueType                         = int64_t;
  using UnsignedType                      = uint64_t;
  using SignedType                        = int64_t;
  using NextSize                          = TypeFromSize<128>;
  static constexpr uint16_t     decimals  = 9;
  static constexpr ValueType    tolerance = 0x200;                 // 0.00000012
  static constexpr ValueType    max_exp   = 0x000000157cd0e6e8LL;  // 21.48756259
  static constexpr UnsignedType min_exp   = 0xffffffea832f1918LL;  // -21.48756259
};

// 32 bit implementation
template <>
struct TypeFromSize<32>
{
  static constexpr bool     is_valid      = true;
  static constexpr uint16_t size          = 32;
  using ValueType                         = int32_t;
  using UnsignedType                      = uint32_t;
  using SignedType                        = int32_t;
  using NextSize                          = TypeFromSize<64>;
  static constexpr uint16_t     decimals  = 4;
  static constexpr ValueType    tolerance = 0x15;         // 0.0003
  static constexpr ValueType    max_exp   = 0x000a65adL;  //  10.3971
  static constexpr UnsignedType min_exp   = 0xfff59a53L;  // -10.3972
};

// 16 bit implementation
template <>
struct TypeFromSize<16>
{
  static constexpr bool     is_valid = true;
  static constexpr uint16_t size     = 16;
  using ValueType                    = int16_t;
  using UnsignedType                 = uint16_t;
  using SignedType                   = int16_t;
  using NextSize                     = TypeFromSize<32>;
};

// 8 bit implementation
template <>
struct TypeFromSize<8>
{
  static constexpr bool     is_valid = true;
  static constexpr uint16_t size     = 8;
  using ValueType                    = int8_t;
  using UnsignedType                 = uint8_t;
  using SignedType                   = int8_t;
  using NextSize                     = TypeFromSize<16>;
};

struct BaseFixedpointType
{
};

template <uint16_t I, uint16_t F>
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

  static constexpr Type FRACTIONAL_MASK = Type((Type(1ull) << FRACTIONAL_BITS) - 1);
  static constexpr Type INTEGER_MASK    = Type(~FRACTIONAL_MASK);
  static constexpr Type ONE_MASK        = Type(1) << FRACTIONAL_BITS;

  ////////////////////////
  /// Constants/Limits ///
  ////////////////////////

  static constexpr Type          SMALLEST_FRACTION{1};
  static constexpr Type          LARGEST_FRACTION{FRACTIONAL_MASK};
  static constexpr Type          MAX_INT{((Type(FRACTIONAL_MASK >> 1) - 1) << FRACTIONAL_BITS)};
  static constexpr Type          MIN_INT{-MAX_INT};
  static constexpr Type          MAX{MAX_INT | LARGEST_FRACTION};
  static constexpr Type          MIN{MIN_INT - LARGEST_FRACTION};
  static constexpr std::uint16_t DECIMAL_DIGITS{BaseTypeInfo::decimals};

  static FixedPoint const TOLERANCE;
  static FixedPoint const _0; /* 0 */
  static FixedPoint const _1; /* 1 */

  static FixedPoint const CONST_SMALLEST_FRACTION;
  static FixedPoint const CONST_E;              /* e */
  static FixedPoint const CONST_LOG2E;          /* log_2 e */
  static FixedPoint const CONST_LOG210;         /* log_2 10 */
  static FixedPoint const CONST_LOG10E;         /* log_10 e */
  static FixedPoint const CONST_LN2;            /* log_e 2 */
  static FixedPoint const CONST_LN10;           /* log_e 10 */
  static FixedPoint const CONST_PI;             /* pi */
  static FixedPoint const CONST_PI_2;           /* pi/2 */
  static FixedPoint const CONST_PI_4;           /* pi/4 */
  static FixedPoint const CONST_INV_PI;         /* 1/pi */
  static FixedPoint const CONST_TWO_INV_PI;     /* 2/pi */
  static FixedPoint const CONST_TWO_INV_SQRTPI; /* 2/sqrt(pi) */
  static FixedPoint const CONST_SQRT2;          /* sqrt(2) */
  static FixedPoint const CONST_INV_SQRT2;      /* 1/sqrt(2) */
  static FixedPoint const MAX_EXP;
  static FixedPoint const MIN_EXP;
  static FixedPoint const FP_MAX;
  static FixedPoint const FP_MIN;
  static FixedPoint const NaN;
  static FixedPoint const POSITIVE_INFINITY;
  static FixedPoint const NEGATIVE_INFINITY;

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

  static constexpr void StateClear();
  static constexpr bool IsState(uint32_t state);
  static constexpr bool IsStateNaN();
  static constexpr bool IsStateUnderflow();
  static constexpr bool IsStateOverflow();
  static constexpr bool IsStateInfinity();
  static constexpr bool IsStateDivisionByZero();

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
  constexpr explicit FixedPoint(T n, meta::IfIsInteger<T> * /*unused*/ = nullptr);
  template <typename T>
  constexpr explicit FixedPoint(T n, meta::IfIsFloat<T> * /*unused*/ = nullptr);
  constexpr FixedPoint(FixedPoint const &o);
  constexpr FixedPoint(Type const &integer, UnsignedType const &fraction);
  template <uint16_t N, uint16_t M>
  constexpr explicit FixedPoint(FixedPoint<N, M> const &o);

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

  ////////////////////////////
  /// assignment operators ///
  ////////////////////////////

  constexpr FixedPoint &operator=(FixedPoint const &o);
  template <typename T>
  constexpr meta::IfIsInteger<T, FixedPoint> &operator=(T const &n);  // NOLINT
  template <typename T>
  constexpr meta::IfIsFloat<T, FixedPoint> &operator=(T const &n);  // NOLINT

  ///////////////////////////////////////////////////
  /// comparison operators for FixedPoint objects ///
  ///////////////////////////////////////////////////

  constexpr bool operator==(FixedPoint const &o) const;
  constexpr bool operator!=(FixedPoint const &o) const;
  constexpr bool operator<(FixedPoint const &o) const;
  constexpr bool operator>(FixedPoint const &o) const;
  constexpr bool operator<=(FixedPoint const &o) const;
  constexpr bool operator>=(FixedPoint const &o) const;
  constexpr bool Near(FixedPoint const &o) const;

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
  template <typename OtherType>
  constexpr bool Near(OtherType const &o) const;

  ///////////////////////
  /// unary operators ///
  ///////////////////////

  constexpr bool        operator!() const;
  constexpr FixedPoint  operator~() const;
  constexpr FixedPoint  operator-() const;
  constexpr FixedPoint &operator++();
  constexpr FixedPoint &operator--();
  FixedPoint const      operator++(int) &;
  FixedPoint const      operator--(int) &;

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
  constexpr FixedPoint &operator<<=(int n);
  constexpr FixedPoint &operator>>=(int n);

  ///////////////////////////
  /// NaN/Infinity checks ///
  ///////////////////////////

  static constexpr bool       IsNaN(FixedPoint const &x);
  static constexpr bool       IsPosInfinity(FixedPoint const &x);
  static constexpr bool       IsNegInfinity(FixedPoint const &x);
  static constexpr bool       IsInfinity(FixedPoint const &x);
  static constexpr FixedPoint infinity(bool isPositive);

  ////////////
  /// swap ///
  ////////////

  constexpr void Swap(FixedPoint &rhs);

  /////////////////////
  /// Getter/Setter ///
  /////////////////////

  constexpr Type        Data() const;
  constexpr Type &      Data();
  constexpr void        SetData(Type n) const;
  constexpr Type const *pointer() const;
  constexpr Type *      pointer();

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

  /**
   * helper function that checks there is no overflow
   * @tparam T the input original type
   * @param n the value of the datum
   * @return true if there is no overflow, false otherwise
   */
  static constexpr bool CheckOverflow(NextType x)
  {
    return (x > NextType(MAX));
  }

  /**
   * helper function that checks there is no underflow
   * @tparam T the input original type
   * @param n the value of the datum
   * @return true if there is no overflow, false otherwise
   */
  static constexpr bool CheckUnderflow(NextType x)
  {
    return (x < NextType(MIN));
  }

private:
  Type data_{0};  // the value to be stored

  static std::function<FixedPoint(FixedPoint const &x)> SinPi2QuadrantFuncs[4];
  static std::function<FixedPoint(FixedPoint const &x)> CosPi2QuadrantFuncs[4];

  // this makes it simpler to create a fixed point object from
  // a native type without scaling
  struct NoScale
  {
  };

  constexpr FixedPoint(Type n, const NoScale & /*unused*/)
    : data_(n)
  {}

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
    return std::numeric_limits<T>::max_digits10 < DECIMAL_DIGITS;
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

  static constexpr FixedPoint         SinPi2(FixedPoint const &r);
  static constexpr FixedPoint         CosPi2(FixedPoint const &r);
  static constexpr FixedPoint<16, 16> SinApproxPi4(FixedPoint<16, 16> const &r);
  static constexpr FixedPoint<32, 32> SinApproxPi4(FixedPoint<32, 32> const &r);
  static constexpr FixedPoint<64, 64> SinApproxPi4(FixedPoint<64, 64> const &r);
  static constexpr FixedPoint         CosApproxPi4(FixedPoint const &r);
};

template <typename T>
static inline bool IsMinusZero(T n);

template <>
inline bool IsMinusZero<float>(float const n)
{
  // Have to dereference the pointer as char const *, else we get a strict-aliasing error
  uint32_t n_int = *(reinterpret_cast<uint8_t const *>(&n));
  return (n_int == 0x80000000);
}

template <>
inline bool IsMinusZero<double>(double const n)
{
  // Have to dereference the pointer as char const *, else we get a strict-aliasing error
  uint64_t n_int = *(reinterpret_cast<uint8_t const *>(&n));
  return (n_int == 0x8000000000000000);
}

template <uint16_t I, uint16_t F>
std::function<FixedPoint<I, F>(FixedPoint<I, F> const &x)>
    FixedPoint<I, F>::SinPi2QuadrantFuncs[4] = {
        [](FixedPoint<I, F> const &x) { return FixedPoint<I, F>::SinPi2(x); },
        [](FixedPoint<I, F> const &x) { return FixedPoint<I, F>::CosPi2(x); },
        [](FixedPoint<I, F> const &x) { return -FixedPoint<I, F>::SinPi2(x); },
        [](FixedPoint<I, F> const &x) { return -FixedPoint<I, F>::CosPi2(x); }};

template <uint16_t I, uint16_t F>
std::function<FixedPoint<I, F>(FixedPoint<I, F> const &x)>
    FixedPoint<I, F>::CosPi2QuadrantFuncs[4] = {
        [](FixedPoint<I, F> const &x) { return FixedPoint<I, F>::CosPi2(x); },
        [](FixedPoint<I, F> const &x) { return -FixedPoint<I, F>::SinPi2(x); },
        [](FixedPoint<I, F> const &x) { return -FixedPoint<I, F>::CosPi2(x); },
        [](FixedPoint<I, F> const &x) { return FixedPoint<I, F>::SinPi2(x); }};

template <uint16_t I, uint16_t F>
uint32_t FixedPoint<I, F>::fp_state{FixedPoint<I, F>::STATE_OK};

template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::SMALLEST_FRACTION;
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::LARGEST_FRACTION;
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::MAX_INT;
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::MIN_INT;
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::MAX;
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::MIN;
template <uint16_t I, uint16_t F>
constexpr uint16_t FixedPoint<I, F>::DECIMAL_DIGITS;

// Instantiate these constants, but define them in the cpp here, earlier compilers need this.

/* e = 2.718281828459045235360287471352662498 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_E;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_E;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_E;

/* log_2(e) = 1.442695040888963407359924681001892137 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LOG2E;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LOG2E;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LOG2E;

/* log_2(10) = 3.32192809488736234787 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LOG210;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LOG210;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LOG210;

/* log_10(e) = 0.434294481903251827651128918916605082 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LOG10E;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LOG10E;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LOG10E;

/* ln(2) = 0.693147180559945309417232121458176568 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LN2;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LN2;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LN2;

/* ln(10) = 2.302585092994045684017991454684364208 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LN10;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LN10;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LN10;

/* Pi = 3.141592653589793238462643383279502884 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_PI;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_PI;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_PI;

/* Pi/2 = 1.570796326794896619231321691639751442 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_PI_2;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_PI_2;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_PI_2;

/* Pi/4 = 0.785398163397448309615660845819875721 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_PI_4;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_PI_4;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_PI_4;

/* 1/Pi = 0.318309886183790671537767526745028724 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_INV_PI;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_INV_PI;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_INV_PI;

/* 2/Pi = 0.636619772367581343075535053490057448 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_TWO_INV_PI;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_TWO_INV_PI;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_TWO_INV_PI;

/* 2/Sqrt(Pi) = 1.128379167095512573896158903121545172 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_TWO_INV_SQRTPI;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_TWO_INV_SQRTPI;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_TWO_INV_SQRTPI;

/* Sqrt(2) = 1.414213562373095048801688724209698079 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_SQRT2;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_SQRT2;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_SQRT2;

/* 1/Sqrt(2) = 0.707106781186547524400844362104849039 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_INV_SQRT2;
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_INV_SQRT2;
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_INV_SQRT2;

template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::_0{0}; /* 0 */
template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::_1{1}; /* 1 */
template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::TOLERANCE(
    0, FixedPoint<I, F>::BaseTypeInfo::tolerance); /* 0 */
template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::CONST_SMALLEST_FRACTION{
    FixedPoint<I, F>(0, FixedPoint<I, F>::SMALLEST_FRACTION)};
template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::MAX_EXP{FixedPoint<I, F>::FromBase(
    FixedPoint<I, F>::BaseTypeInfo::max_exp)}; /* maximum exponent for Exp() */
template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::MIN_EXP{
    FixedPoint<I, F>::FromBase(static_cast<FixedPoint<I, F>::Type>(
        FixedPoint<I, F>::BaseTypeInfo::min_exp))}; /* minimum exponent for Exp() */
template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::FP_MAX{FixedPoint<I, F>::FromBase(FixedPoint<I, F>::MAX)};
template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::FP_MIN{FixedPoint<I, F>::FromBase(FixedPoint<I, F>::MIN)};
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::NaN{
    FixedPoint<I, F>::FromBase(Type(1) << (TOTAL_BITS - 1) | FRACTIONAL_MASK)};
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::POSITIVE_INFINITY{
    FixedPoint<I, F>::FromBase(FixedPoint<I, F>::MAX + FixedPoint<I, F>::ONE_MASK)};
template <std::uint16_t I, std::uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::NEGATIVE_INFINITY{
    FixedPoint<I, F>::FromBase(FixedPoint<I, F>::MIN - FixedPoint<I, F>::ONE_MASK)};

inline std::ostream &operator<<(std::ostream &s, int128_t const &x)
{
  s << static_cast<uint64_t>(x >> 64) << "|" << static_cast<uint64_t>(x);
  return s;
}

template <uint16_t I, uint16_t F>
inline std::ostream &operator<<(std::ostream &s, FixedPoint<I, F> const &n)
{
  std::ios_base::fmtflags f(s.flags());
  s << std::setfill('0');
  s << std::setw(I / 4);
  s << std::setprecision(F / 4);
  s << std::fixed;
  if (FixedPoint<I, F>::IsNaN(n))
  {
    s << "NaN";
  }
  else if (FixedPoint<I, F>::IsPosInfinity(n))
  {
    s << "+∞";
  }
  else if (FixedPoint<I, F>::IsNegInfinity(n))
  {
    s << "-∞";
  }
  else
  {
    s << double(n);
  }
#ifdef FETCH_FIXEDPOINT_DEBUG_HEX
  // Only output the hex value in DEBUG mode
  s << " (0x" << std::hex << static_cast<typename FixedPoint<I, F>::Type>(n.Data()) << ")";
#endif
  s.flags(f);
  return s;
}

///////////////////////////
/// State of operations ///
///////////////////////////

template <uint16_t I, uint16_t F>
constexpr void FixedPoint<I, F>::StateClear()
{
  fp_state = 0;
}

template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsState(uint32_t const state)
{
  return (fp_state & state) != 0u;
}

template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsStateNaN()
{
  return IsState(STATE_NAN);
}

template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsStateUnderflow()
{
  return IsState(STATE_UNDERFLOW);
}

template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsStateOverflow()
{
  return IsState(STATE_OVERFLOW);
}

template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsStateInfinity()
{
  return IsState(STATE_INFINITY);
}

template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsStateDivisionByZero()
{
  return IsState(STATE_DIVISION_BY_ZERO);
}

////////////////////
/// constructors ///
////////////////////

/**
 * Templated constructor when T is an integer type
 * @tparam T any integer type
 * @param n integer value to set FixedPoint to
 */
template <uint16_t I, uint16_t F>
template <typename T>
constexpr FixedPoint<I, F>::FixedPoint(T n, meta::IfIsInteger<T> * /*unused*/)
  : data_{static_cast<typename FixedPoint<I, F>::Type>(n)}
{
  if (CheckOverflow(static_cast<NextType>(n)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MAX;
  }
  else if (CheckUnderflow(static_cast<NextType>(n)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MIN;
  }
  else
  {
    Type s    = (data_ < 0) - (data_ > 0);
    Type abs_ = s * data_;
    abs_ <<= FRACTIONAL_BITS;
    data_ = s * abs_;
  }
}

/**
 * Templated constructor only when T is a float/double type
 * @tparam T any float/double type
 * @param n float/double value to set FixedPoint to
 */
template <uint16_t I, uint16_t F>
template <typename T>
constexpr FixedPoint<I, F>::FixedPoint(T n, meta::IfIsFloat<T> * /*unused*/)
  : data_(static_cast<typename FixedPoint<I, F>::Type>(n * ONE_MASK))
{
  if (IsMinusZero(n))
  {
    data_ = 0;
  }
  else if (CheckOverflow(static_cast<NextType>(n * ONE_MASK)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MAX;
  }
  else if (CheckUnderflow(static_cast<NextType>(n * ONE_MASK)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MIN;
  }
}

/**
 * Copy constructor
 * @param o other FixedPoint object to copy
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F>::FixedPoint(FixedPoint<I, F> const &o)
  : data_{o.data_}
{}

/**
 * Templated constructor taking 2 arguments, integer and fraction part
 * @param integer
 * @param fraction
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F>::FixedPoint(typename FixedPoint<I, F>::Type const &        integer,
                                       typename FixedPoint<I, F>::UnsignedType const &fraction)
  : data_{(INTEGER_MASK & (Type(integer) << FRACTIONAL_BITS)) | Type(fraction & FRACTIONAL_MASK)}
{}

template <>
template <>
constexpr FixedPoint<32, 32>::FixedPoint(FixedPoint<16, 16> const &o)
  : data_{static_cast<int64_t>(o.Data()) << 16}
{}

template <>
template <>
constexpr FixedPoint<64, 64>::FixedPoint(FixedPoint<16, 16> const &o)
  : data_{static_cast<int128_t>(o.Data()) << 48}
{}

template <>
template <>
constexpr FixedPoint<64, 64>::FixedPoint(FixedPoint<32, 32> const &o)
  : data_{static_cast<int128_t>(o.Data()) << 32}
{}

template <>
template <>
constexpr FixedPoint<32, 32>::FixedPoint(FixedPoint<64, 64> const &o)
  : data_{static_cast<int64_t>(o.Data() >> 32)}
{
  if (CheckOverflow(static_cast<NextType>(data_)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MAX;
  }
  else if (CheckUnderflow(static_cast<NextType>(data_)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MIN;
  }
}

template <>
template <>
constexpr FixedPoint<16, 16>::FixedPoint(FixedPoint<64, 64> const &o)
  : data_{static_cast<int32_t>(o.Data() >> 48)}
{
  if (CheckOverflow(static_cast<NextType>(data_)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MAX;
  }
  else if (CheckUnderflow(static_cast<NextType>(data_)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MIN;
  }
}

template <>
template <>
constexpr FixedPoint<16, 16>::FixedPoint(FixedPoint<32, 32> const &o)
  : data_{static_cast<int32_t>(o.Data() >> 16)}
{
  if (CheckOverflow(static_cast<NextType>(data_)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MAX;
  }
  else if (CheckUnderflow(static_cast<NextType>(data_)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MIN;
  }
}

///////////////////
/// conversions ///
///////////////////

/**
 * Method to return the integer part of the FixedPoint object
 * @return the integer part of the FixedPoint object
 */
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::Integer() const
{
  if (IsNaN(*this))
  {
    fp_state |= STATE_NAN;
  }
  return Type((data_ & INTEGER_MASK) >> FRACTIONAL_BITS);
}

/**
 * Method to return the fraction part of the FixedPoint object
 * @return the fraction part of the FixedPoint object
 */
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::Fraction() const
{
  if (IsNaN(*this))
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Floor(FixedPoint<I, F> const &o)
{
  if (IsNaN(o))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  return FixedPoint{o.Integer()};
}

/**
 * Return the nearest greater integer to the FixedPoint number o
 * @param the FixedPoint object to use Round() on
 * @return the nearest greater integer to the FixedPoint number o
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Round(FixedPoint<I, F> const &o)
{
  if (IsNaN(o))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  return Floor(o + FixedPoint{0.5});
}

/**
 * Take a raw primitive of Type and convert it to a FixedPoint object
 * @param the primitive to convert to FixedPoint
 * @return the converted FixedPoint
 */
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
FixedPoint<I, F>::operator double() const
{
  return (static_cast<double>(data_) / ONE_MASK);
}

/**
 * Cast the FixedPoint object to a float primitive
 * @return the cast FixedPoint object to float
 */
template <uint16_t I, uint16_t F>
FixedPoint<I, F>::operator float() const
{
  return (static_cast<float>(data_) / ONE_MASK);
}

/**
 * Cast the FixedPoint object to an integer primitive
 * @return the cast FixedPoint object to an integer primitive
 */
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator=(FixedPoint<I, F> const &o)
{
  if (IsNaN(o))
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
template <uint16_t I, uint16_t F>  // NOLINT
template <typename T>
constexpr meta::IfIsInteger<T, FixedPoint<I, F>> &FixedPoint<I, F>::operator=(T const &n)  // NOLINT
{
  if (CheckOverflow(static_cast<NextType>(n)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MAX;
  }
  else if (CheckUnderflow(static_cast<NextType>(n)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MIN;
  }
  else
  {
    data_ = {static_cast<Type>(n) << static_cast<Type>(FRACTIONAL_BITS)};
  }
  return *this;
}

/**
 * Assignment operator for primitive float types
 * @param the primitive to assign the FixedPoint object from
 * @return copies the given primitive float n
 */
template <uint16_t I, uint16_t F>  // NOLINT
template <typename T>
constexpr meta::IfIsFloat<T, FixedPoint<I, F>> &FixedPoint<I, F>::operator=(T const &n)  // NOLINT
{
  if (CheckOverflow(static_cast<NextType>(n * ONE_MASK)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MAX;
  }
  else if (CheckUnderflow(static_cast<NextType>(n * ONE_MASK)))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MIN;
  }
  else
  {
    data_ = static_cast<typename FixedPoint<I, F>::Type>(n * ONE_MASK);
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
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::operator==(FixedPoint const &o) const
{
  if (IsNaN(*this) || IsNaN(o))
  {
    return false;
  }

  return (data_ == o.Data());
}

/**
 * Inequality comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if objects are unequal, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::operator!=(FixedPoint const &o) const
{
  if (IsNaN(*this) || IsNaN(o))
  {
    return true;
  }

  return (data_ != o.Data());
}

/**
 * Less than comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is less than o, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::operator<(FixedPoint const &o) const
{
  if (IsNaN(*this) || IsNaN(o))
  {
    return false;
  }
  if (IsNegInfinity(*this))
  {
    // Negative infinity is always smaller than all other quantities except itself
    return !IsNegInfinity(o);
  }
  if (IsPosInfinity(*this))
  {
    // Positive infinity is never smaller than any other quantity
    return false;
  }

  if (IsNegInfinity(o))
  {
    return false;
  }
  if (IsPosInfinity(o))
  {
    return true;
  }

  return (data_ < o.Data());
}

/**
 * Less than or equal comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is less than or equal to o, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::operator<=(FixedPoint const &o) const
{
  return (*this < o) || (*this == o);
}

/**
 * Greater than comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is greater than o, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::operator>(FixedPoint const &o) const
{
  return (o < *this);
}

/**
 * Greater than or equal comparison operator, note, NaN objects are never equal to each other
 * @param the FixedPoint object to compare to
 * @return true if object is greater than or equal to o, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::operator>=(FixedPoint const &o) const
{
  return (o < *this) || (*this == o);
}

/**
 * Check if two numbers are near according to abs(*this - o) < tolerance
 * @param the FixedPoint object to compare to
 * @return true if object is close to o, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::Near(FixedPoint const &o) const
{
  return (Abs(*this - o) < TOLERANCE);
}

////////////////////////////////////////////////
/// comparison operators against other types ///
////////////////////////////////////////////////

/**
 * Equality comparison operator, note, NaN objects are never equal to each other
 * @param the primitive object to compare to
 * @return true if objects are equal, false otherwise
 */
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::operator>=(OtherType const &o) const
{
  return (*this >= FixedPoint(o));
}

/**
 * Check if two numbers are near according to abs(*this - o) < tolerance
 * @param the FixedPoint object to compare to
 * @return true if object is close to o, false otherwise
 */
template <uint16_t I, uint16_t F>
template <typename OtherType>
constexpr bool FixedPoint<I, F>::Near(OtherType const &o) const
{
  return Near(FixedPoint{o});
}

///////////////////////
/// unary operators ///
///////////////////////

/**
 * Unary minus operator
 * @return the negative number
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator-() const
{
  if (IsNaN(*this))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsPosInfinity(*this))
  {
    fp_state |= STATE_INFINITY;
    return NEGATIVE_INFINITY;
  }
  if (IsNegInfinity(*this))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }
  FixedPoint t(*this);
  t.data_ = -t.data_;
  return t;
}

/**
 * Logical Negation operator, note, NaN objects are never equal to each other
 * @return true if object is greater than o, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::operator!() const
{
  return !data_;
}

/**
 * Bitwise NOT operator
 * @return the bitwise NOT of the number
 */
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator++()
{
  if (CheckOverflow(static_cast<NextType>(data_) + static_cast<NextType>(_1.Data())))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MAX;
  }
  else if (CheckUnderflow(static_cast<NextType>(data_) + static_cast<NextType>(_1.Data())))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MIN;
  }
  else
  {
    data_ += ONE_MASK;
  }
  return *this;
}

/**
 * Prefix decrement operator, decrease the number by one
 * @return the number decreased by one, prefix mode
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator--()
{
  if (CheckOverflow(static_cast<NextType>(data_) - static_cast<NextType>(_1.Data())))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MAX;
  }
  else if (CheckUnderflow(static_cast<NextType>(data_) - static_cast<NextType>(_1.Data())))
  {
    fp_state |= STATE_OVERFLOW;
    data_ = MIN;
  }
  else
  {
    data_ -= ONE_MASK;
  }
  return *this;
}

/**
 * Postfix increment operator, increase the number by one
 * @return the number increased by one, prefix mode
 */
template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::operator++(int) &
{
  FixedPoint<I, F> result{*this};
  ++result;
  return result;
}

/**
 * Postfix decrement operator, decrease the number by one
 * @return the number decreased by one, prefix mode
 */
template <uint16_t I, uint16_t F>
FixedPoint<I, F> const FixedPoint<I, F>::operator--(int) &
{
  FixedPoint<I, F> result{*this};
  --result;
  return result;
}

/////////////////////////////////////////////////
/// math operators against FixedPoint objects ///
/////////////////////////////////////////////////

/**
 * Addition operator
 * @param the FixedPoint object to add to
 * @return the sum of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator+(FixedPoint<I, F> const &n) const
{
  FixedPoint res{*this};
  res += n;
  return res;
}

/**
 * Subtraction operator
 * @param the FixedPoint object to subtract from
 * @return the difference of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator-(FixedPoint<I, F> const &n) const
{
  FixedPoint res{*this};
  res -= n;
  return res;
}

/**
 * Multiplication operator
 * @param the FixedPoint object to multiply against
 * @return the product of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator*(FixedPoint<I, F> const &n) const
{
  FixedPoint res{*this};
  res *= n;
  return res;
}

/**
 * Division operator
 * @param the FixedPoint object to divide against
 * @return the division of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator/(FixedPoint<I, F> const &n) const
{
  FixedPoint res{*this};
  res /= n;
  return res;
}

/**
 * Bitwise AND operator, does bitwise AND between the given FixedPoint object and self
 * @param the given FixedPoint object to AND against
 * @return the bitwise AND operation between the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator&(FixedPoint<I, F> const &n) const
{
  FixedPoint t{*this};
  t &= n;
  return t;
}

/**
 * Bitwise OR operator, does bitwise OR between the given FixedPoint object and self
 * @param the given FixedPoint object to OR against
 * @return the bitwise OR operation between the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator|(FixedPoint<I, F> const &n) const
{
  FixedPoint t{*this};
  t |= n;
  return t;
}

/**
 * Bitwise XOR operator, does bitwise XOR between the given FixedPoint object and self
 * @param the given FixedPoint object to XOR against
 * @return the bitwise XOR operation between the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator^(FixedPoint<I, F> const &n) const
{
  FixedPoint t{*this};
  t ^= n;
  return t;
}

/**
 * Addition assignment operator, adds given object to self
 * @param the given FixedPoint object to add to
 * @return the sum of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator+=(FixedPoint<I, F> const &n)
{
  if (IsNaN(*this) || IsNaN(n))
  {
    fp_state |= STATE_NAN;
    *this = NaN;
  }
  else if (IsPosInfinity(*this))
  {
    // Adding +∞ to -∞ gives NaN, +∞ otherwise
    if (IsNegInfinity(n))
    {
      fp_state |= STATE_NAN;
      *this = NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      *this = POSITIVE_INFINITY;
    }
  }
  else if (IsNegInfinity(*this))
  {
    // Adding +∞ to -∞ gives NaN, -∞ otherwise
    if (IsPosInfinity(n))
    {
      fp_state |= STATE_NAN;
      *this = NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      *this = NEGATIVE_INFINITY;
    }
  }
  else
  {
    if (IsPosInfinity(n))
    {
      fp_state |= STATE_INFINITY;
      *this = POSITIVE_INFINITY;
    }
    else if (IsNegInfinity(n))
    {
      fp_state |= STATE_INFINITY;
      *this = NEGATIVE_INFINITY;
    }
    else if (CheckOverflow(static_cast<NextType>(data_) + static_cast<NextType>(n.Data())))
    {
      fp_state |= STATE_OVERFLOW;
      data_ = MAX;
    }
    else if (CheckUnderflow(static_cast<NextType>(data_) + static_cast<NextType>(n.Data())))
    {
      fp_state |= STATE_OVERFLOW;
      data_ = MIN;
    }
    else
    {
      Type fp = data_ + n.Data();
      *this   = FromBase(fp);
    }
  }
  return *this;
}

/**
 * Subtraction assignment operator, subtracts given object from self
 * @param the given FixedPoint object to subtract from
 * @return the difference of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator-=(FixedPoint<I, F> const &n)
{

  if (IsNaN(*this) || IsNaN(n))
  {
    fp_state |= STATE_NAN;
    *this = NaN;
  }
  else if (IsPosInfinity(*this))
  {
    // Subtracting -∞ from +∞ gives NaN, +∞ otherwise
    if (IsPosInfinity(n))
    {
      fp_state |= STATE_NAN;
      *this = NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      *this = POSITIVE_INFINITY;
    }
  }
  else if (IsNegInfinity(*this))
  {
    // Subtracting -∞ from -∞ gives NaN, -∞ otherwise
    if (IsNegInfinity(n))
    {
      fp_state |= STATE_NAN;
      *this = NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      *this = NEGATIVE_INFINITY;
    }
  }
  else if (IsPosInfinity(n))
  {
    fp_state |= STATE_INFINITY;
    *this = NEGATIVE_INFINITY;
  }
  else if (IsNegInfinity(n))
  {
    fp_state |= STATE_INFINITY;
    *this = POSITIVE_INFINITY;
  }
  else
  {
    if (IsNegInfinity(n))
    {
      fp_state |= STATE_INFINITY;
      *this = POSITIVE_INFINITY;
    }
    else if (IsPosInfinity(n))
    {
      fp_state |= STATE_INFINITY;
      *this = NEGATIVE_INFINITY;
    }
    else if (CheckOverflow(static_cast<NextType>(data_) - static_cast<NextType>(n.Data())))
    {
      fp_state |= STATE_OVERFLOW;
      data_ = MAX;
    }
    else if (CheckUnderflow(static_cast<NextType>(data_) - static_cast<NextType>(n.Data())))
    {
      fp_state |= STATE_OVERFLOW;
      data_ = MIN;
    }
    else
    {
      Type fp = data_ - n.Data();
      *this   = FromBase(fp);
    }
  }
  return *this;
}

/**
 * Multiplication assignment operator, multiply given object to self
 * @param the given FixedPoint object to multiply against
 * @return the product of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator*=(FixedPoint<I, F> const &n)
{
  if (IsNaN(*this) || IsNaN(n))
  {
    fp_state |= STATE_NAN;
    *this = NaN;
  }
  else if (IsInfinity(*this) && IsInfinity(n))
  {
    fp_state |= STATE_INFINITY;
    *this = infinity((IsPosInfinity(*this) && IsPosInfinity(n)) ||
                     (IsNegInfinity(*this) && IsNegInfinity(n)));
  }
  else if (IsInfinity(*this))
  {
    if (n == _0)
    {
      // ∞ * 0 is NaN
      fp_state |= STATE_NAN;
      *this = NaN;
    }
    else
    {
      // Normal number, return +/-∞ depending on the sign of n
      fp_state |= STATE_INFINITY;
      *this = infinity((IsPosInfinity(*this) && n > _0) || (IsNegInfinity(*this) && n < _0));
    }
  }
  else if (IsInfinity(n))
  {
    if (*this == _0)
    {
      // 0 * ∞ is NaN
      fp_state |= STATE_NAN;
      *this = NaN;
    }
    else
    {
      // Normal number, return +/-∞ depending on the sign of *this
      fp_state |= STATE_INFINITY;
      *this = infinity((*this > _0 && IsPosInfinity(n)) || (*this < _0 && IsNegInfinity(n)));
    }
  }
  else
  {
    NextType prod = NextType(data_) * NextType(n.Data());
    if (CheckOverflow(static_cast<NextType>(prod >> size_t(FRACTIONAL_BITS))))
    {
      fp_state |= STATE_OVERFLOW;
      data_ = MAX;
    }
    else if (CheckUnderflow(static_cast<NextType>(prod >> size_t(FRACTIONAL_BITS))))
    {
      fp_state |= STATE_OVERFLOW;
      data_ = MIN;
    }
    else
    {
      auto fp = static_cast<Type>(prod >> size_t(FRACTIONAL_BITS));
      *this   = FromBase(fp);
    }
  }
  return *this;
}

/**
 * Division assignment operator, divide self against given object
 * @param the given FixedPoint object to divide against
 * @return the division of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator/=(FixedPoint<I, F> const &n)
{
  if (IsNaN(*this) || IsNaN(n))
  {
    fp_state |= STATE_NAN;
    *this = NaN;
  }
  else if (n == _0)
  {
    if (*this == _0)
    {
      fp_state |= STATE_NAN;
      *this = NaN;
    }
    else
    {
      fp_state |= STATE_DIVISION_BY_ZERO;
      *this = NaN;
    }
  }
  else if (IsInfinity(*this))
  {
    if (IsInfinity(n))
    {
      fp_state |= STATE_NAN;
      *this = NaN;
    }
    else
    {
      fp_state |= STATE_INFINITY;
      *this = infinity((IsPosInfinity(*this) && n > _0) || (IsNegInfinity(*this) && n < 0));
    }
  }
  else
  {
    FixedPoint sign        = Sign(*this);
    FixedPoint abs_n       = Abs(*this);
    auto       numerator   = NextType(abs_n.Data()) << size_t(FRACTIONAL_BITS);
    auto       denominator = NextType(n.Data());
    NextType   quotient    = numerator / denominator;
    *this                  = sign * FromBase(Type(quotient));
  }
  return *this;
}

/**
 * Bitwise AND assignment operator, does bitwise AND between the given FixedPoint object and self
 * @param the given FixedPoint object to AND against
 * @return the bitwise AND operation between the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator+(T const &n) const
{
  return *this + FixedPoint(n);
}

/**
 * Subtraction operator
 * @param the primitive object to subtract from
 * @return the difference of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator-(T const &n) const
{
  return *this - FixedPoint(n);
}

/**
 * Multiplication operator against primitive object
 * @param the primitive number to multiply against
 * @return the product of the two numbers
 */
template <uint16_t I, uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator*(T const &n) const
{
  return *this * FixedPoint(n);
}

/**
 * Division operator
 * @param the primitive number to multiply against
 * @return the division of the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator/(T const &n) const
{
  return *this / FixedPoint(n);
}

/**
 * Bitwise AND assignment operator, does bitwise AND between the given FixedPoint object and self
 * @param the given FixedPoint object to AND against
 * @return the bitwise AND operation between the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator&(T const &n) const
{
  return *this & FixedPoint(n);
}

/**
 * Bitwise OR assignment operator, does bitwise OR between the given FixedPoint object and self
 * @param the given FixedPoint object to OR against
 * @return the bitwise OR operation between the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator|(T const &n) const
{
  return *this | FixedPoint(n);
}

/**
 * Bitwise XOR assignment operator, does bitwise XOR between the given FixedPoint object and self
 * @param the given FixedPoint object to XOR against
 * @return the bitwise XOR operation between the two FixedPoint objects
 */
template <uint16_t I, uint16_t F>
template <typename T>
constexpr FixedPoint<I, F> FixedPoint<I, F>::operator^(T const &n) const
{
  return *this ^ FixedPoint(n);
}

/**
 * Shift right assignment operator, shift self object right as many bits as Integer() part of the
 * given object
 * @param the given object
 * @return the result of the shift right operation
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator>>=(int n)
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> &FixedPoint<I, F>::operator<<=(int n)
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
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsNaN(FixedPoint<I, F> const &x)
{
  return x.Data() == NaN.Data();
}

/**
 * Check if given FixedPoint object is +infinity
 * @param the given object to check for +infinity
 * @return true if x is +infinity, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsPosInfinity(FixedPoint<I, F> const &x)
{
  return x.Data() == POSITIVE_INFINITY.Data();
}

/**
 * Check if given FixedPoint object is -infinity
 * @param the given object to check for -infinity
 * @return true if x is -infinity, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsNegInfinity(FixedPoint<I, F> const &x)
{
  return x.Data() == NEGATIVE_INFINITY.Data();
}

/**
 * Check if given FixedPoint object is +/- infinity
 * @param the given object to check for infinity
 * @return true if x is +/- infinity, false otherwise
 */
template <uint16_t I, uint16_t F>
constexpr bool FixedPoint<I, F>::IsInfinity(FixedPoint<I, F> const &x)
{
  return IsPosInfinity(x) || IsNegInfinity(x);
}

/**
 * helper function to return +∞ is isPositive is true -∞ otherwise
 * @param true if +∞ is needed, false otherwise
 * @return +∞ is isPositive is true -∞ otherwise
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::infinity(bool const isPositive)
{
  return isPositive ? POSITIVE_INFINITY : NEGATIVE_INFINITY;
}

////////////
/// swap ///
////////////

/**
 * Swap two FixedPoint objects
 * @param the secondary object to swap self with
 */
template <uint16_t I, uint16_t F>
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
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type FixedPoint<I, F>::Data() const
{
  return data_;
}

/**
 * Return the contents of the FixedPoint object
 * @return the contents of the FixedPoint object
 */
template <std::uint16_t I, std::uint16_t F>
constexpr typename FixedPoint<I, F>::Type &FixedPoint<I, F>::Data()
{
  return data_;
}

/**
 * Set the contents of the FixedPoint object
 * @param the new contents
 */
template <uint16_t I, uint16_t F>
constexpr void FixedPoint<I, F>::SetData(typename FixedPoint<I, F>::Type const n) const
{
  data_ = n;
}

/**
 * Return a pointer to the contents of the FixedPoint object
 * @return a pointer to the contents of the FixedPoint object
 */
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type const *FixedPoint<I, F>::pointer() const
{
  return &data_;
}

/**
 * Return a pointer to the contents of the FixedPoint object
 * @return a pointer to the contents of the FixedPoint object
 */
template <uint16_t I, uint16_t F>
constexpr typename FixedPoint<I, F>::Type *FixedPoint<I, F>::pointer()
{
  return &data_;
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Remainder(FixedPoint<I, F> const &x,
                                                       FixedPoint<I, F> const &y)
{
  FixedPoint result = x / y;
  return x - Round(result) * y;
}

/**
 * Return the result Fmod of the division between x/y
 * @param the numerator x
 * @param the denominator y
 * @return the result of the Fmod operation
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Fmod(FixedPoint<I, F> const &x,
                                                  FixedPoint<I, F> const &y)
{
  FixedPoint result = Remainder(Abs(x), Abs(y));
  if (result < _0)
  {
    result += Abs(y);
  }
  return Sign(x) * result;
}

/**
 * Return the absolute value of FixedPoint number x
 * @param the given x
 * @return the absolute value of x
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Abs(FixedPoint<I, F> const &x)
{
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }

  return x * Sign(x);
}

/**
 * Return the sign of a FixedPoint number
 * @param the given x
 * @return 1 if x is positive or zero, -1 if x is negative
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Sign(FixedPoint<I, F> const &x)
{
  return FixedPoint{Type((x >= _0) - (x < _0))};
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

static constexpr FixedPoint<64, 64> Exp_P01(0, 0x8000000000000000);  //  1 / 2
static constexpr FixedPoint<64, 64> Exp_P02(0, 0x1C71C71C71C71C71);  //  1 / 9
static constexpr FixedPoint<64, 64> Exp_P03(0, 0x038E38E38E38E38E);  //  1 / 72
static constexpr FixedPoint<64, 64> Exp_P04(0, 0x0041041041041041);  //  1 / 1008
static constexpr FixedPoint<64, 64> Exp_P05(0, 0x00022ACD578022AC);  //  1 / 30240

template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Exp(FixedPoint<I, F> const &x)
{
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsNegInfinity(x))
  {
    return _0;
  }
  if (IsPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }
  if (x < MIN_EXP)
  {
    return _0;
  }
  if (x > MAX_EXP)
  {
    fp_state |= STATE_OVERFLOW;
    return FP_MAX;
  }
  if (x == _1)
  {
    return CONST_E;
  }
  if (x == _0)
  {
    return _1;
  }
  if (x < _0)
  {
    return _1 / Exp(-x);
  }

  // Find integer k and r ∈ [-0.5, 0.5) such as: x = k*ln2 + r
  // Then exp(x) = 2^k * e^r
  FixedPoint k = x / CONST_LN2;
  k            = Floor(k);

  FixedPoint r = x - k * CONST_LN2;
  FixedPoint e1{_1};
  e1 <<= k;

  FixedPoint r2 = r * r;
  FixedPoint r3 = r2 * r;
  FixedPoint r4 = r3 * r;
  FixedPoint r5 = r4 * r;
  // Multiply the coefficients as they are the same in both numerator and denominator
  r *= static_cast<FixedPoint>(Exp_P01);
  r2 *= static_cast<FixedPoint>(Exp_P02);
  r3 *= static_cast<FixedPoint>(Exp_P03);
  r4 *= static_cast<FixedPoint>(Exp_P04);
  r5 *= static_cast<FixedPoint>(Exp_P05);
  FixedPoint P  = _1 + r + r2 + r3 + r4 + r5;
  FixedPoint Q  = _1 - r + r2 - r3 + r4 - r5;
  FixedPoint e2 = P / Q;

  return e1 * e2;
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Log2(FixedPoint<I, F> const &x)
{
  if (x == _1)
  {
    return _0;
  }
  if (x == _0)
  {
    fp_state |= STATE_INFINITY;
    return NEGATIVE_INFINITY;
  }
  if (x == CONST_SMALLEST_FRACTION)
  {
    return FixedPoint{-FRACTIONAL_BITS};
  }
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }
  if (x < _0)
  {
    fp_state |= STATE_NAN;
    return NaN;
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
  Type       k = platform::HighestSetBit(y.Data()) - Type(FRACTIONAL_BITS);
  FixedPoint k_shifted{FixedPoint::FromBase((_1.Data()) << k)};
  FixedPoint r = y / k_shifted;

  FixedPoint P00{137};
  FixedPoint P01{1762};  // P03 also
  FixedPoint P02{3762};
  FixedPoint P04{137};
  FixedPoint Q0{30};
  FixedPoint Q01{24};  // Q03 also
  FixedPoint Q02{76};
  FixedPoint P = (-_1 + r) * (P00 + r * (P01 + r * (P02 + r * (P01 + r * P04))));
  FixedPoint Q = Q0 * (_1 + r) * (_1 + r * (Q01 + r * (Q02 + r * (Q01 + r)))) * CONST_LN2;
  FixedPoint R = P / Q;

  return sign * (FixedPoint(k) + R);
}

/**
 * Calculate the FixedPoint natural logarithm log(x).
 * Return the Log2(x) / Log2(e)
 * @param the given x
 * @return the result of log(x)
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Log(FixedPoint<I, F> const &x)
{
  return Log2(x) / CONST_LOG2E;
}

/**
 * Calculate the FixedPoint base10 logarithm log(x).
 * Return the Log2(x) / Log2(10)
 * @param the given x
 * @return the result of log10(x)
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Log10(FixedPoint<I, F> const &x)
{
  return Log2(x) / CONST_LOG210;
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Sqrt(FixedPoint<I, F> const &x)
{
  if (x == _1)
  {
    return _1;
  }
  if (x == _0)
  {
    return _0;
  }
  if (x < _0)
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
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

  return twok * r;
}

/**
 * Calculate the FixedPoint power x^y
 *
 * Special cases
 * * x or y is NaN    -> pow(x, y) = NaN
 * * x == 0, y == 0   -> pow(x, y) = NaN
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Pow(FixedPoint<I, F> const &x,
                                                 FixedPoint<I, F> const &y)
{
  if (IsNaN(x) || IsNaN(y))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (y == _0)
  {
    return _1;
  }
  if (y == _1)
  {
    if (IsInfinity(x))
    {
      fp_state |= STATE_INFINITY;
    }
    return x;
  }
  if (IsPosInfinity(x))
  {
    if (y > _0)
    {
      fp_state |= STATE_INFINITY;
      return POSITIVE_INFINITY;
    }

    return _0;
  }
  if (IsNegInfinity(x))
  {
    return Pow(_0, -y);
  }
  if (x == _0)
  {
    if (y < _0)
    {
      fp_state |= STATE_NAN;

      return NaN;
    }

    return _0;
  }
  if (IsPosInfinity(y))
  {
    if (Abs(x) > _1)
    {
      fp_state |= STATE_INFINITY;
      return POSITIVE_INFINITY;
    }
    if (Abs(x) == _1)
    {
      return _1;
    }

    return _0;
  }
  if (IsNegInfinity(y))
  {
    if (Abs(x) > _1)
    {
      return _0;
    }
    if (Abs(x) == _1)
    {
      return _1;
    }

    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }
  if (x < _0)
  {
    FixedPoint x1{x};
    if (y.Fraction() == 0)
    {
      if (y < _0)
      {
        x1 = _1 / x;
      }
      FixedPoint pow{x1};
      FixedPoint t{Abs(y)};
      while (--t)
      {
        pow *= x1;
      }
      return pow;
    }

    // Already checked the case for integer y
    fp_state |= STATE_NAN;
    return NaN;
  }

  FixedPoint s = _1 * ((y.Integer() + 1) & 1) + Sign(x) * (y.Integer() & 1);
  return s * Exp(y * Log(Abs(x)));
}

template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::SinPi2(FixedPoint<I, F> const &r)
{
  assert(r <= CONST_PI_2);

  if (r > CONST_PI_4)
  {
    return CosPi2(r - CONST_PI_2);
  }

  return SinApproxPi4(r);
}

template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::CosPi2(FixedPoint<I, F> const &r)
{
  assert(r <= CONST_PI_2);

  if (r > CONST_PI_4)
  {
    return SinPi2(CONST_PI_2 - r);
  }

  return CosApproxPi4(r);
}

template <>
constexpr FixedPoint<16, 16> FixedPoint<16, 16>::SinApproxPi4(FixedPoint<16, 16> const &r)
{
  assert(r <= CONST_PI_4);

  FixedPoint<16, 16> r2 = r * r;
  FixedPoint<16, 16> r3 = r2 * r;
  FixedPoint<16, 16> r4 = r3 * r;
  FixedPoint<16, 16> Q00{5880};
  FixedPoint<16, 16> P = r * Q00 - r3 * 620;
  FixedPoint<16, 16> Q = Q00 + r2 * 360 + r4 * 11;
  return P / Q;
}

template <>
constexpr FixedPoint<32, 32> FixedPoint<32, 32>::SinApproxPi4(FixedPoint<32, 32> const &r)
{
  assert(r <= CONST_PI_4);

  FixedPoint<32, 32> r2 = r * r;
  FixedPoint<32, 32> r3 = r2 * r;
  FixedPoint<32, 32> r4 = r3 * r;
  FixedPoint<32, 32> r5 = r4 * r;
  FixedPoint<32, 32> Q00{166320};
  FixedPoint<32, 32> P = r * Q00 - r3 * 22260 + r5 * 551;
  FixedPoint<32, 32> Q = Q00 + r2 * 5460 + r4 * 75;
  return P / Q;
}

template <>
constexpr FixedPoint<64, 64> FixedPoint<64, 64>::SinApproxPi4(FixedPoint<64, 64> const &r)
{
  assert(r <= CONST_PI_4);

  FixedPoint<64, 64> r2 = r * r;
  FixedPoint<64, 64> r3 = r2 * r;
  FixedPoint<64, 64> r4 = r3 * r;
  FixedPoint<64, 64> r5 = r4 * r;
  FixedPoint<64, 64> Q00{166320};
  FixedPoint<64, 64> P   = r * Q00 - r3 * 22260 + r5 * 551;
  FixedPoint<64, 64> Q   = Q00 + r2 * 5460 + r4 * 75;
  FixedPoint<64, 64> sin = P / Q;

  return sin;
}

template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::CosApproxPi4(FixedPoint<I, F> const &r)
{
  assert(r <= CONST_PI_4);

  FixedPoint<I, F> r2 = r * r;
  FixedPoint<I, F> r4 = r2 * r2;
  FixedPoint<I, F> Q00{15120};
  FixedPoint<I, F> P   = Q00 - r2 * 6900 + r4 * 313;
  FixedPoint<I, F> Q   = Q00 + r2 * 660 + r4 * 13;
  FixedPoint<I, F> cos = P / Q;

  return cos;
}

/**
 * Calculate the FixedPoint sine of x sin(x).
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Sin(FixedPoint<I, F> const &x)
{
  if (IsNaN(x) || IsInfinity(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (x < _0)
  {
    return -Sin(-x);
  }

  FixedPoint r = Fmod(x, CONST_PI * 2);

  if (r == _0)
  {
    return _0;
  }

  FixedPoint quadrant = Floor(r / CONST_PI_2);

  return SinPi2QuadrantFuncs[static_cast<std::size_t>(quadrant)](r - CONST_PI_2 * quadrant);
}

/**
 * Calculate the FixedPoint cosine of x cos(x).
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Cos(FixedPoint<I, F> const &x)
{
  if (IsNaN(x) || IsInfinity(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }

  FixedPoint r = Fmod(Abs(x), CONST_PI * 2);
  if (r == _0)
  {
    return _1;
  }

  FixedPoint quadrant = Floor(r / CONST_PI_2);

  return CosPi2QuadrantFuncs[static_cast<std::size_t>(quadrant)](r - CONST_PI_2 * quadrant);
}

/**
 * Calculate the FixedPoint tangent of x tan(x).
 *
 * Special cases
 * * x is NaN    -> tan(NaN) = NaN
 * * x == 1      -> tan(x) = 1
 * * x == 0      -> tan(x) = 0
 * * x < 0       -> tan(x) = NaN
 * * x == +inf   -> tan(+inf) = +inf
 *
 * Calculation of tan(x) is done by doing range reduction of x in the range [0, 2*PI]
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

static constexpr FixedPoint<64, 64> Tan_P01(0, 0x1F07C1F07C1F07C1);  //  4 / 33
static constexpr FixedPoint<64, 64> Tan_P02(0, 0x0084655D9BAB2F10);  //  1 / 495
static constexpr FixedPoint<64, 64> Tan_Q01(0, 0x745D1745D1745D17);  //  5 / 11
static constexpr FixedPoint<64, 64> Tan_Q02(0, 0x052BF5A814AFD6A0);  //  2 / 99
static constexpr FixedPoint<64, 64> Tan_Q03(0, 0x00064DF8445D7C25);  //  1 / 10395

template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::Tan(FixedPoint<I, F> const &x)
{
  if (IsNaN(x) || IsInfinity(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (x == CONST_PI_2)
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }
  if (x == -CONST_PI_2)
  {
    fp_state |= STATE_INFINITY;
    return NEGATIVE_INFINITY;
  }
  if (x < _0)
  {
    return -Tan(-x);
  }

  FixedPoint r   = Fmod(x, CONST_PI);
  auto       P01 = static_cast<FixedPoint>(-Tan_P01);  // -4 / 33
  auto       P02 = static_cast<FixedPoint>(Tan_P02);   //  1 / 495
  auto       Q01 = static_cast<FixedPoint>(-Tan_Q01);  // -5 / 11
  auto       Q02 = static_cast<FixedPoint>(Tan_Q02);   //  2 / 99
  auto       Q03 = static_cast<FixedPoint>(-Tan_Q03);  // -1 / 10395
  if (r <= CONST_PI_4)
  {
    FixedPoint r2 = r * r;
    FixedPoint P  = r * (_1 + r2 * (P01 + r2 * P02));
    FixedPoint Q  = _1 + r2 * (Q01 + r2 * (Q02 + r2 * Q03));
    return P / Q;
  }
  if (r < CONST_PI_2)
  {
    FixedPoint y  = r - CONST_PI_2;
    FixedPoint y2 = y * y;
    FixedPoint P  = -(_1 + y2 * (Q01 + y2 * (Q02 + y2 * Q03)));
    FixedPoint Q  = -CONST_PI_2 + r + y2 * y * (P01 + y2 * P02);
    return P / Q;
  }

  return Tan(r - CONST_PI);
}

/**
 * Calculate the FixedPoint arcsin of x asin(x).
 * Based on the NetBSD libm asin() implementation
 *
 * Special cases
 * * x is NaN    -> asin(x) = NaN
 * * x is +/-inf -> asin(x) = NaN
 * * |x| > 1     -> asin(x) = NaN
 * * x < 0       -> asin(x) = -asin(-x)
 *
 * errors for x ∈ (-1, 1):
 * FixedPoint<16,16>: average: 1.76928e-05, max: 0.000294807
 * FixedPoint<32,32>: average: 2.62396e-10, max: 1.87484e-09
 *
 * @param the given x
 * @return the result of asin(x)
 */

static constexpr FixedPoint<64, 64> ASin_P00(0, 0x2AAAAAAAAAAAAA00);  //  1.66666666666666657415e-01
static constexpr FixedPoint<64, 64> ASin_P01(0, 0x5358480FADBDF3FF);  //  3.25565818622400915405e-01
static constexpr FixedPoint<64, 64> ASin_P02(0, 0x3382AA1D1088A9FF);  //  2.01212532134862925881e-01
static constexpr FixedPoint<64, 64> ASin_P03(0, 0x0A41145AB4479D80);  //  4.00555345006794114027e-02
static constexpr FixedPoint<64, 64> ASin_P04(0, 0x0033DFC0EA036510);  //  7.91534994289814532176e-04
static constexpr FixedPoint<64, 64> ASin_P05(0, 0x000247BC21BFBEE1);  //  3.47933107596021167570e-05
static constexpr FixedPoint<64, 64> ASin_Q01(2, 0x6744E39145A95FFF);  //  2.40339491173441421878e+00
static constexpr FixedPoint<64, 64> ASin_Q02(2, 0x055CB38B3158FFFF);  //  2.02094576023350569471e+00
static constexpr FixedPoint<64, 64> ASin_Q03(0, 0xB03360DC680AC7FF);  //  6.88283971605453293030e-01
static constexpr FixedPoint<64, 64> ASin_Q04(0, 0x13B8C5B12E9281FF);  //  7.70381505559019352791e-02

template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ASin(FixedPoint<I, F> const &x)
{
  if (IsNaN(x) || IsInfinity(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (x < _0)
  {
    return -ASin(-x);
  }
  if (x > _1)
  {
    fp_state |= STATE_NAN;
    return NaN;
  }

  auto       P00 = static_cast<FixedPoint>(ASin_P00);   //  1.66666666666666657415e-01
  auto       P01 = static_cast<FixedPoint>(-ASin_P01);  // -3.25565818622400915405e-01
  auto       P02 = static_cast<FixedPoint>(ASin_P02);   //  2.01212532134862925881e-01
  auto       P03 = static_cast<FixedPoint>(-ASin_P03);  // -4.00555345006794114027e-02
  auto       P04 = static_cast<FixedPoint>(ASin_P04);   //  7.91534994289814532176e-04
  auto       P05 = static_cast<FixedPoint>(ASin_P05);   //  3.47933107596021167570e-05
  auto       Q01 = static_cast<FixedPoint>(-ASin_Q01);  // -2.40339491173441421878e+00
  auto       Q02 = static_cast<FixedPoint>(ASin_Q02);   //  2.02094576023350569471e+00
  auto       Q03 = static_cast<FixedPoint>(-ASin_Q03);  // -6.88283971605453293030e-01
  auto       Q04 = static_cast<FixedPoint>(ASin_Q04);   //  7.70381505559019352791e-02
  FixedPoint c;
  if (x < 0.5)
  {
    FixedPoint t = x * x;
    FixedPoint P = t * (P00 + t * (P01 + t * (P02 + t * (P03 + t * (P04 + t * P05)))));
    FixedPoint Q = _1 + t * (Q01 + t * (Q02 + t * (Q03 + t * Q04)));
    FixedPoint R = P / Q;
    return x + x * R;
  }

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
    Q = CONST_PI_4 - w * 2.0;
    t = CONST_PI_4 - (P - Q);
    return t;
  }

  w = P / Q;
  t = CONST_PI_2 - ((s + s * R) * 2.0);
  return t;
}

/**
 * Calculate the FixedPoint arccos of x acos(x).
 *
 * Special cases
 * * x is NaN    -> acos(x) = NaN
 * * x is +/-inf -> acos(x) = NaN
 * * |x| > 1     -> acos(x) = NaN
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ACos(FixedPoint<I, F> const &x)
{
  if (Abs(x) > _1)
  {
    fp_state |= STATE_NAN;
    return NaN;
  }

  return CONST_PI_2 - ASin(x);
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

static constexpr FixedPoint<64, 64> ATan_P03(2, 0x08FB823EE08FB824);  //  116 / 57
static constexpr FixedPoint<64, 64> ATan_P05(1, 0x5C69E32684D5AD41);  // 2198 / 1615
static constexpr FixedPoint<64, 64> ATan_P07(0, 0x54B1152C454B1152);  //   44 / 133
static constexpr FixedPoint<64, 64> ATan_P09(0, 0x056A97A719C012E3);  // 5597 / 264537
static constexpr FixedPoint<64, 64> ATan_Q02(2, 0x5E50D79435E50D79);  //   45 / 19
static constexpr FixedPoint<64, 64> ATan_Q04(1, 0xF351A27A0E442936);  //  630 / 323
static constexpr FixedPoint<64, 64> ATan_Q06(0, 0xA6708B7E04C16312);  //  210 / 323
static constexpr FixedPoint<64, 64> ATan_Q08(0, 0x13345EDD4F51640B);  //  315 / 4199
static constexpr FixedPoint<64, 64> ATan_Q10(0, 0x0059637863300679);  //   63 / 46189

template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ATan(FixedPoint<I, F> const &x)
{
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsPosInfinity(x))
  {
    return CONST_PI_2;
  }
  if (IsNegInfinity(x))
  {
    return -CONST_PI_2;
  }
  if (x < _0)
  {
    return -ATan(-x);
  }
  if (x > _1)
  {
    return CONST_PI_2 - ATan(_1 / x);
  }

  auto       P03 = static_cast<FixedPoint>(ATan_P03);  //  116 / 57
  auto       P05 = static_cast<FixedPoint>(ATan_P05);  // 2198 / 1615
  auto       P07 = static_cast<FixedPoint>(ATan_P07);  //   44 / 133
  auto       P09 = static_cast<FixedPoint>(ATan_P09);  // 5597 / 264537
  auto       Q02 = static_cast<FixedPoint>(ATan_Q02);  //   45 / 19
  auto       Q04 = static_cast<FixedPoint>(ATan_Q04);  //  630 / 323
  auto       Q06 = static_cast<FixedPoint>(ATan_Q06);  //  210 / 323
  auto       Q08 = static_cast<FixedPoint>(ATan_Q08);  //  315 / 4199
  auto       Q10 = static_cast<FixedPoint>(ATan_Q10);  //   63 / 46189
  FixedPoint x2  = x * x;
  FixedPoint P   = x * (_1 + x2 * (P03 + x2 * (P05 + x2 * (P07 + x2 * P09))));
  FixedPoint Q   = _1 + x2 * (Q02 + x2 * (Q04 + x2 * (Q06 + x2 * (Q08 + x2 * Q10))));

  return P / Q;
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ATan2(FixedPoint<I, F> const &y,
                                                   FixedPoint<I, F> const &x)
{
  if (IsNaN(y) || IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsPosInfinity(y))
  {
    if (IsPosInfinity(x))
    {
      return CONST_PI_4;
    }
    if (IsNegInfinity(x))
    {
      return CONST_PI_4 * 3;
    }

    return CONST_PI_2;
  }
  if (IsNegInfinity(y))
  {
    if (IsPosInfinity(x))
    {
      return -CONST_PI_4;
    }
    if (IsNegInfinity(x))
    {
      return -CONST_PI_4 * 3;
    }

    return -CONST_PI_2;
  }
  if (IsPosInfinity(x))
  {
    return _0;
  }
  if (IsNegInfinity(x))
  {
    return Sign(y) * CONST_PI;
  }

  if (y < _0)
  {
    return -ATan2(-y, x);
  }

  if (x == _0)
  {
    return Sign(y) * CONST_PI_2;
  }

  FixedPoint u    = y / Abs(x);
  FixedPoint atan = ATan(u);
  if (x < _0)
  {
    return CONST_PI - atan;
  }

  return atan;
}

/**
 * Calculate the FixedPoint hyperbolic sine of x sinh(x).
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::SinH(FixedPoint<I, F> const &x)
{
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }
  if (IsNegInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return NEGATIVE_INFINITY;
  }

  FixedPoint half{0.5};
  return half * (Exp(x) - Exp(-x));
}

/**
 * Calculate the FixedPoint hyperbolic cosine of x cosh(x).
 *
 * Special cases
 * * x is NaN    -> cosh(x) = NaN
 * * x is +/-inf -> cosh(x) = +inf
 * Calculated using the definition formula:
 *
 *            e^x + e^(-x)
 * cosh(x) = --------------
 *                2
 *
 * errors for x ∈ [-3, 3):
 * FixedPoint<16,16>: average: 6.92127e-05, max: 0.000487532
 * errors for x ∈ [-5, 5):
 * FixedPoint<32,32>: average: 7.30786e-09, max: 7.89509e-08

 * @param the given x
 * @return the result of cosh(x)
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::CosH(FixedPoint<I, F> const &x)
{
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }

  FixedPoint half{0.5};
  return half * (Exp(x) + Exp(-x));
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
 * tanh(x) = --------------
 *            e^x + e^(-x)
 *
 * errors for x ∈ [-3, 3):
 * FixedPoint<16,16>: average: 1.25046e-05, max: 7.0897e-05
 * FixedPoint<32,32>: average: 1.7648e-10,  max: 1.19186e-09

 * @param the given x
 * @return the result of tanh(x)
 */
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::TanH(FixedPoint<I, F> const &x)
{
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }
  if (IsNegInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return NEGATIVE_INFINITY;
  }

  FixedPoint e1 = Exp(x);
  FixedPoint e2 = Exp(-x);
  return (e1 - e2) / (e1 + e2);
}

/**
 * Calculate the FixedPoint inverse of hyperbolic sine of x, asinh(x).
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ASinH(FixedPoint<I, F> const &x)
{
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }
  if (IsNegInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return NEGATIVE_INFINITY;
  }

  return Log(x + Sqrt(x * x + _1));
}

/**
 * Calculate the FixedPoint inverse of hyperbolic cosine of x, acosh(x).
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ACosH(FixedPoint<I, F> const &x)
{
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsPosInfinity(x))
  {
    fp_state |= STATE_INFINITY;
    return POSITIVE_INFINITY;
  }
  if (IsNegInfinity(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }

  // ACosH(x) is defined in the [1, +inf) range
  if (x < _1)
  {
    return NaN;
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
template <uint16_t I, uint16_t F>
constexpr FixedPoint<I, F> FixedPoint<I, F>::ATanH(FixedPoint<I, F> const &x)
{
  if (IsNaN(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }
  if (IsInfinity(x))
  {
    fp_state |= STATE_NAN;
    return NaN;
  }

  // ATanH(x) is defined in the (-1, 1) range
  if (Abs(x) > _1)
  {
    return NaN;
  }
  FixedPoint half{0.5};
  return half * Log((_1 + x) / (_1 - x));
}

}  // namespace fixed_point
}  // namespace fetch
