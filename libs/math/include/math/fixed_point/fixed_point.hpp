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
  constexpr explicit FixedPoint(T n, meta::IfIsInteger<T> * = nullptr)
    : data_{static_cast<Type>(n)}
  {
    assert(CheckNoOverflow(n));
    Type s    = (data_ < 0) - (data_ > 0);
    Type abs_ = s * data_;
    abs_ <<= FRACTIONAL_BITS;
    data_ = s * abs_;
  }

  template <typename T>
  constexpr explicit FixedPoint(T n, meta::IfIsFloat<T> * = nullptr)
    : data_(static_cast<Type>(n * ONE_MASK))
  {
    assert(CheckNoOverflow(n * ONE_MASK));
    //    assert(CheckNoRounding<T>());
  }

  constexpr FixedPoint(FixedPoint const &o)
    : data_{o.data_}
  {}

  constexpr FixedPoint(const Type &integer, const UnsignedType &fraction)
    : data_{(INTEGER_MASK & (Type(integer) << FRACTIONAL_BITS)) | Type(fraction & FRACTIONAL_MASK)}
  {}

  constexpr Type Integer() const
  {
    if (isNaN(*this))
    {
      throw std::overflow_error("Cannot get the integer part of a NaN value!");
    }
    return Type((data_ & INTEGER_MASK) >> FRACTIONAL_BITS);
  }

  constexpr Type Fraction() const
  {
    if (isNaN(*this))
    {
      throw std::overflow_error("Cannot get the fraction part of a NaN value!");
    }
    return (data_ & FRACTIONAL_MASK);
  }

  static constexpr FixedPoint Floor(FixedPoint const &o)
  {
    if (isNaN(o))
    {
      throw std::overflow_error("Cannot use Floor() on a NaN value!");
    }
    return std::move(FixedPoint{o.Integer()});
  }

  static constexpr FixedPoint Round(FixedPoint const &o)
  {
    if (isNaN(o))
    {
      throw std::overflow_error("Cannot use Floor() on a NaN value!");
    }
    return std::move(Floor(o + FixedPoint{0.5}));
  }

  /////////////////
  /// operators ///
  /////////////////

  constexpr FixedPoint &operator=(FixedPoint const &o)
  {
    if (isNaN(o))
    {
      throw std::overflow_error("Cannot assing NaN value!");
    }
    data_ = o.data_;
    return *this;
  }

  template <typename T>
  constexpr FixedPoint &operator=(T const &n)
  {
    data_ = {static_cast<Type>(n) << static_cast<Type>(FRACTIONAL_BITS)};
    assert(CheckNoOverflow(n));
    return *this;
  }

  ///////////////////////////////////////////////////
  /// comparison operators for FixedPoint objects ///
  ///////////////////////////////////////////////////

  constexpr bool operator==(FixedPoint const &o) const
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

  constexpr bool operator!=(FixedPoint const &o) const
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

  constexpr bool operator<(FixedPoint const &o) const
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

  constexpr bool operator>(FixedPoint const &o) const
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

  template <typename OtherType>
  constexpr bool operator==(OtherType const &o) const
  {
    return (data_ == FixedPoint(o).Data());
  }

  template <typename OtherType>
  constexpr bool operator!=(OtherType const &o) const
  {
    return (data_ != FixedPoint(o).Data());
  }

  template <typename OtherType>
  constexpr bool operator<(OtherType const &o) const
  {
    return (data_ < FixedPoint(o).Data());
  }

  template <typename OtherType>
  constexpr bool operator>(OtherType const &o) const
  {
    return (data_ > FixedPoint(o).Data());
  }

  template <typename OtherType>
  constexpr bool operator<=(OtherType const &o) const
  {
    return (data_ <= FixedPoint(o).Data());
  }

  template <typename OtherType>
  constexpr bool operator>=(OtherType const &o) const
  {
    return (data_ >= FixedPoint(o).Data());
  }

  ///////////////////////
  /// unary operators ///
  ///////////////////////

  constexpr bool operator!() const
  {
    return !data_;
  }

  constexpr FixedPoint operator~() const
  {
    FixedPoint t(*this);
    t.data_ = ~t.data_;
    return t;
  }

  constexpr FixedPoint operator-() const
  {
    FixedPoint t(*this);
    t.data_ = -t.data_;
    return t;
  }

  constexpr FixedPoint &operator++()
  {
    assert(CheckNoOverflow(data_ + CONST_ONE.Data()));
    data_ += ONE_MASK;
    return *this;
  }

  constexpr FixedPoint &operator--()
  {
    assert(CheckNoOverflow(data_ - CONST_ONE.Data()));
    data_ -= ONE_MASK;
    return *this;
  }

  /////////////////////////
  /// casting operators ///
  /////////////////////////

  explicit operator double() const
  {
    return (static_cast<double>(data_) / ONE_MASK);
  }

  explicit operator float() const
  {
    return (static_cast<float>(data_) / ONE_MASK);
  }

  template <typename T>
  explicit operator T() const
  {
    return (static_cast<T>(Integer()));
  }

  //////////////////////
  /// math operators ///
  //////////////////////

  constexpr FixedPoint operator+(FixedPoint const &n) const
  {
    assert(CheckNoOverflow(data_ + n.Data()));
    Type fp = data_ + n.Data();
    return FromBase(fp);
  }

  template <typename T>
  constexpr FixedPoint operator+(T const &n) const
  {
    return FixedPoint(T(data_) + n);
  }

  constexpr FixedPoint operator-(FixedPoint const &n) const
  {
    assert(CheckNoOverflow(data_ - n.Data()));
    Type fp = data_ - n.Data();
    return FromBase(fp);
  }

  template <typename T>
  constexpr FixedPoint operator-(T const &n) const
  {
    return FixedPoint(T(data_) - n);
  }

  constexpr FixedPoint operator*(FixedPoint const &n) const
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

  template <typename T>
  constexpr FixedPoint operator*(T const &n) const
  {
    return *this * FixedPoint(n);
  }

  constexpr FixedPoint operator/(FixedPoint const &n) const
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

  template <typename T>
  constexpr FixedPoint operator/(T const &n) const
  {
    return *this / FixedPoint(n);
  }

  constexpr FixedPoint &operator+=(FixedPoint const &n)
  {
    assert(CheckNoOverflow(data_ + n.Data()));
    data_ += n.Data();
    return *this;
  }

  constexpr FixedPoint &operator-=(FixedPoint const &n)
  {
    assert(CheckNoOverflow(data_ - n.Data()));
    data_ -= n.Data();
    return *this;
  }

  constexpr FixedPoint &operator&=(FixedPoint const &n)
  {
    assert(CheckNoOverflow(data_ & n.Data()));
    data_ &= n.Data();
    return *this;
  }

  constexpr FixedPoint &operator|=(FixedPoint const &n)
  {
    assert(CheckNoOverflow(data_ | n.Data()));
    data_ |= n.Data();
    return *this;
  }

  constexpr FixedPoint &operator^=(FixedPoint const &n)
  {
    assert(CheckNoOverflow(data_ ^ n.Data()));
    data_ ^= n.Data();
    return *this;
  }

  constexpr FixedPoint &operator*=(FixedPoint const &n)
  {
    Multiply(*this, n, *this);
    return *this;
  }

  constexpr FixedPoint &operator/=(FixedPoint const &n)
  {
    FixedPoint temp;
    *this = Divide(*this, n, temp);
    return *this;
  }

  constexpr FixedPoint &operator>>=(FixedPoint const &n)
  {
    data_ >>= n.Integer();
    return *this;
  }

  constexpr FixedPoint &operator>>=(const int &n)
  {
    data_ >>= n;
    return *this;
  }

  constexpr FixedPoint &operator<<=(FixedPoint const &n)
  {
    data_ <<= n.Integer();
    return *this;
  }

  constexpr FixedPoint &operator<<=(const int &n)
  {
    data_ <<= n;
    return *this;
  }

  ////////////
  /// swap ///
  ////////////

  constexpr void Swap(FixedPoint &rhs)
  {
    Type tmp = data_;
    data_    = rhs.Data();
    rhs.SetData(tmp);
  }

  /////////////////////
  /// Getter/Setter ///
  /////////////////////

  constexpr Type Data() const
  {
    return data_;
  }

  constexpr void SetData(Type const n) const
  {
    data_ = n;
  }

  //////////////////////////////////////////////////////
  /// FixedPoint implementations of common functions ///
  //////////////////////////////////////////////////////

  static constexpr FixedPoint Exp(FixedPoint const &x)
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
    // Multiply the coefficients as they are the same in both numerator and denominator
    r *= FixedPoint(0.5);
    r2 *= FixedPoint(3.0 / 28.0);
    r3 *= CONST_ONE / FixedPoint(84.0);
    r4 *= CONST_ONE / FixedPoint(1680.0);
    FixedPoint P  = CONST_ONE + r + r2 + r3 + r4;
    FixedPoint Q  = CONST_ONE - r + r2 - r3 + r4;
    FixedPoint e2 = P / Q;

    return std::move(e1 * e2);
  }

  static constexpr FixedPoint Log2(FixedPoint const &x)
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
      throw std::runtime_error("Log2(): mathematical operation not defined: x < 0!");
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

    // Pade Approximant for Log2(x) around x=1, 4th order
    FixedPoint P00{5};
    FixedPoint P01{37};
    FixedPoint Q00{6};
    FixedPoint Q01{16};
    FixedPoint Q02{36};
    FixedPoint f2 = f * f;
    FixedPoint f3 = f2 * f;
    FixedPoint f4 = f3 * f;
    FixedPoint P  = P00 * (-CONST_ONE + f) * (P00 + P01 * f + P01 * f2 + P00 * f3);
    FixedPoint Q  = Q00 * (CONST_ONE + f * Q01 + f2 * Q02 + f3 * Q01 + f4) * CONST_LN2;
    FixedPoint R  = P / Q;

    if (adjustment)
    {
      return std::move(-FixedPoint(k) - R);
    }
    else
    {
      return std::move(FixedPoint(k) + R);
    }
  }

  static constexpr FixedPoint Log(FixedPoint const &x)
  {
    return Log2(x) / CONST_LOG2E;
  }

  static constexpr FixedPoint Log10(FixedPoint const &x)
  {
    return Log2(x) / CONST_LOG210;
  }

  static constexpr FixedPoint Sqrt(FixedPoint const &x)
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
      throw std::runtime_error("Sqrt(): mathematical operation not defined: x < 0!");
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

  static constexpr FixedPoint Pow(FixedPoint const &x, FixedPoint const &y)
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
    if (x < CONST_ZERO && y.Fraction() != 0)
    {
      throw std::runtime_error(
          "Pow(x, y): x^y where x < 0 and y non-integer: mathematical operation not defined!");
    }
    FixedPoint s   = CONST_ONE * ((y.Integer() + 1) & 1) + Sign(x) * (y.Integer() & 1);
    FixedPoint pow = s * Exp(y * Log(Abs(x)));
    return std::move(pow);
  }

  static constexpr FixedPoint Sin(FixedPoint const &x)
  {
    if (x < CONST_ZERO)
    {
      return -Sin(-x);
    }

    FixedPoint r = ReducePi(x);

    if (r == CONST_ZERO)
    {
      return CONST_ZERO;
    }

    FixedPoint quadrant = Floor(r / CONST_PI_2);

    return SinPi2QuadrantFuncs[(size_t)quadrant](r - CONST_PI_2 * quadrant);
  }

  static constexpr FixedPoint Cos(FixedPoint const &x)
  {
    if (x < CONST_ZERO)
    {
      return Cos(-x);
    }

    FixedPoint r = ReducePi(x);

    if (r == CONST_ZERO)
    {
      return CONST_ONE;
    }

    FixedPoint quadrant = Floor(r / CONST_PI_2);

    return CosPi2QuadrantFuncs[(size_t)quadrant](r - CONST_PI_2 * quadrant);
  }

  static constexpr FixedPoint Tan(FixedPoint const &x)
  {
    return Sin(x) / Cos(x);
  }

  static constexpr FixedPoint Remainder(FixedPoint const &x, FixedPoint const &y)
  {
    FixedPoint result = x / y;
    return std::move(x - Round(result) * y);
  }

  static constexpr FixedPoint Fmod(FixedPoint const &x, FixedPoint const &y)
  {
    FixedPoint result = Remainder(Abs(x), Abs(y));
    if (result < CONST_ZERO)
    {
      result += Abs(y);
    }
    return std::move(Sign(x) * result);
  }

  static constexpr FixedPoint Abs(FixedPoint const &x)
  {
    return std::move(x * Sign(x));
  }

  static constexpr FixedPoint Sign(FixedPoint const &x)
  {
    return std::move(FixedPoint{Type((x > CONST_ZERO) - (x < CONST_ZERO))});
  }

  static constexpr FixedPoint FromBase(Type n)
  {
    return FixedPoint(n, NoScale());
  }

  static constexpr bool isNaN(FixedPoint const &x)
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

  static constexpr FixedPoint ReducePi(FixedPoint const &x)
  {
    return std::move(Fmod(x, CONST_PI * 2));
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

}  // namespace fixed_point
}  // namespace fetch
