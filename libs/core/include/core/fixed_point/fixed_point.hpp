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

#include <iostream>
#include <sstream>

#include <cmath>
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
  static constexpr bool          is_valid = true;
  static constexpr std::uint16_t size     = 64;
  using ValueType                         = int64_t;
  using UnsignedType                      = uint64_t;
  using SignedType                        = int64_t;
  using NextSize                          = TypeFromSize<128>;
};

// 32 bit implementation
template <>
struct TypeFromSize<32>
{
  static constexpr bool          is_valid = true;
  static constexpr std::uint16_t size     = 32;
  using ValueType                         = int32_t;
  using UnsignedType                      = uint32_t;
  using SignedType                        = int32_t;
  using NextSize                          = TypeFromSize<64>;
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
inline FixedPoint<I, F> Divide(const FixedPoint<I, F> &numerator,
                               const FixedPoint<I, F> &denominator,
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
inline void Multiply(const FixedPoint<I, F> &lhs, const FixedPoint<I, F> &rhs,
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
    ONE_MASK        = Type(1) << FRACTIONAL_BITS
  };

  ////////////////////////
  /// Constants/Limits ///
  ////////////////////////

  static const FixedPoint CONST_ZERO; /* 0 */
  static const FixedPoint CONST_ONE;  /* 1 */
  static const FixedPoint CONST_SMALLEST_FRACTION;
  static const FixedPoint CONST_E;            /* e */
  static const FixedPoint CONST_LOG2E;        /* log_2 e */
  static const FixedPoint CONST_LOG210;       /* log_2 10 */
  static const FixedPoint CONST_LOG10E;       /* log_10 e */
  static const FixedPoint CONST_LN2;          /* log_e 2 */
  static const FixedPoint CONST_LN10;         /* log_e 10 */
  static const FixedPoint CONST_PI;           /* pi */
  static const FixedPoint CONST_PI_2;         /* pi/2 */
  static const FixedPoint CONST_PI_4;         /* pi/4 */
  static const FixedPoint CONST_INV_PI;       /* 1/pi */
  static const FixedPoint CONST_2_INV_PI;     /* 2/pi */
  static const FixedPoint CONST_2_INV_SQRTPI; /* 2/sqrt(pi) */
  static const FixedPoint CONST_SQRT2;        /* sqrt(2) */
  static const FixedPoint CONST_INV_SQRT2;    /* 1/sqrt(2) */
  // Note: The primitive constants should be turned into constexpr
  static const Type       SMALLEST_FRACTION;
  static const Type       LARGEST_FRACTION;
  static const Type       MAX_INT;
  static const Type       MIN_INT;
  static const Type       MAX;
  static const Type       MIN;
  static const FixedPoint MAX_EXP;
  static const FixedPoint MIN_EXP;

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
    // assert(CheckNoOverflow(n, FRACTIONAL_BITS, TOTAL_BITS));
    Type s    = (data_ < 0) - (data_ > 0);
    Type abs_ = s * data_;
    abs_ <<= FRACTIONAL_BITS;
    data_ = s * abs_;
  }

  template <typename T>
  constexpr explicit FixedPoint(T n, meta::IfIsFloat<T> * = nullptr)
    : data_(static_cast<Type>(n * ONE_MASK))
  {
    // assert(CheckNoOverflow(n, FRACTIONAL_BITS, TOTAL_BITS));
    // assert(CheckNoRounding(n, FRACTIONAL_BITS));
  }

  constexpr FixedPoint(const FixedPoint &o)
    : data_{o.data_}
  {}

  constexpr FixedPoint(const Type &integer, const UnsignedType &fraction)
    : data_{(INTEGER_MASK & (Type(integer) << FRACTIONAL_BITS)) | Type(fraction & FRACTIONAL_MASK)}
  {}

  constexpr Type integer() const
  {
    return Type((data_ & INTEGER_MASK) >> FRACTIONAL_BITS);
  }

  constexpr Type fraction() const
  {
    return (data_ & FRACTIONAL_MASK);
  }

  constexpr FixedPoint floor() const
  {
    return FixedPoint(Type((data_ & INTEGER_MASK) >> FRACTIONAL_BITS));
  }

  /////////////////
  /// operators ///
  /////////////////

  constexpr FixedPoint &operator=(const FixedPoint &o)
  {
    data_ = o.data_;
    return *this;
  }

  template <typename T>
  constexpr FixedPoint &operator=(const T &n)
  {
    data_ = {static_cast<Type>(n) << static_cast<Type>(FRACTIONAL_BITS)};
    // assert(CheckNoOverflow(n, FRACTIONAL_BITS, TOTAL_BITS));
  }

  ////////////////////////////
  /// comparison operators ///
  ////////////////////////////

  template <typename OtherType>
  constexpr bool operator==(const OtherType &o) const
  {
    return (data_ == FixedPoint(o).data_);
  }

  template <typename OtherType>
  constexpr bool operator!=(const OtherType &o) const
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
  constexpr bool operator<=(const OtherType &o) const
  {
    return data_ <= FixedPoint(o).data_;
  }

  template <typename OtherType>
  constexpr bool operator>=(const OtherType &o) const
  {
    return data_ >= FixedPoint(o).data_;
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
    data_ += ONE_MASK;
    return *this;
  }

  constexpr FixedPoint &operator--()
  {
    data_ -= CONST_ONE;
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
    return (static_cast<T>(integer()));
  }

  //////////////////////
  /// math operators ///
  //////////////////////

  constexpr FixedPoint operator+(const FixedPoint &n) const
  {
    Type fp = data_ + n.Data();
    return FromBase(fp);
  }

  template <typename T>
  constexpr FixedPoint operator+(const T &n) const
  {
    return FixedPoint(T(data_) + n);
  }

  constexpr FixedPoint operator-(const FixedPoint &n) const
  {
    Type fp = data_ - n.Data();
    return FromBase(fp);
  }

  template <typename T>
  constexpr FixedPoint operator-(const T &n) const
  {
    return FixedPoint(T(data_) - n);
  }

  constexpr FixedPoint operator*(const FixedPoint &n) const
  {
    NextType prod = NextType(data_) * NextType(n.Data());
    Type     fp   = Type(prod >> FRACTIONAL_BITS);
    return FromBase(fp);
  }

  template <typename T>
  constexpr FixedPoint operator*(const T &n) const
  {
    return *this * FixedPoint(n);
  }

  constexpr FixedPoint operator/(const FixedPoint &n) const
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
  constexpr FixedPoint operator/(const T &n) const
  {
    return *this / FixedPoint(n);
  }

  constexpr FixedPoint &operator+=(const FixedPoint &n)
  {
    data_ += n.Data();
    return *this;
  }

  constexpr FixedPoint &operator-=(const FixedPoint &n)
  {
    data_ -= n.Data();
    return *this;
  }

  constexpr FixedPoint &operator&=(const FixedPoint &n)
  {
    data_ &= n.Data();
    return *this;
  }

  constexpr FixedPoint &operator|=(const FixedPoint &n)
  {
    data_ |= n.Data();
    return *this;
  }

  constexpr FixedPoint &operator^=(const FixedPoint &n)
  {
    data_ ^= n.Data();
    return *this;
  }

  constexpr FixedPoint &operator*=(const FixedPoint &n)
  {
    Multiply(*this, n, *this);
    return *this;
  }

  constexpr FixedPoint &operator/=(const FixedPoint &n)
  {
    FixedPoint temp;
    *this = Divide(*this, n, temp);
    return *this;
  }

  constexpr FixedPoint &operator>>=(const FixedPoint &n)
  {
    data_ >>= n.integer();
    return *this;
  }

  constexpr FixedPoint &operator<<=(const FixedPoint &n)
  {
    data_ <<= n.integer();
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

  constexpr Type Data() const
  {
    return data_;
  }

  constexpr void SetData(const Type n) const
  {
    data_ = n;
  }

  static constexpr FixedPoint Exp(const FixedPoint &x)
  {
    if (x < MIN_EXP)
    {
      return CONST_ZERO;
    }
    if (x > MAX_EXP)
    {
      throw std::overflow_error("Exp() does not support exponents larger than MAX_EXP");
    }
    if (x == CONST_ONE)
    {
      return CONST_E;
    }
    if (x == CONST_ZERO)
    {
      return CONST_ONE;
    }
    if (x < CONST_ZERO)
    {
      return CONST_ONE / Exp(-x);
    }

    // Find integer k and r ∈ [-0.5, 0.5) such as: x = k*ln2 + r
    // Then exp(x) = 2^k * e^r
    FixedPoint k = x / CONST_LN2;
    k            = k.floor();

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

    return e1 * e2;
  }

  static constexpr FixedPoint Log2(const FixedPoint &x)
  {
    if (x == CONST_ONE)
    {
      return CONST_ZERO;
    }
    if (x == CONST_ZERO)
    {
      return CONST_ONE;
    }
    if (x.Data() == SMALLEST_FRACTION)
    {
      return FixedPoint{-FRACTIONAL_BITS};
    }
    if (x < CONST_ZERO)
    {
      throw std::runtime_error("Log2(): mathematical operation not defined: x < 0!");
    }
    // TODO(private 843): Add proper support for NaN/Infinity/etc in FixedPoint class
    /*If (x == NaN) {
      return NaN;
    }*/

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
      return -FixedPoint(k) - R;
    }
    else
    {
      return FixedPoint(k) + R;
    }
  }

  static constexpr FixedPoint Log(const FixedPoint &x)
  {
    return Log2(x) / CONST_LOG2E;
  }

  static constexpr FixedPoint Log10(const FixedPoint &x)
  {
    return Log2(x) / CONST_LOG210;
  }

  static constexpr FixedPoint Sqrt(const FixedPoint &x)
  {
    return FixedPoint(std::sqrt((double)x));
  }

  static constexpr FixedPoint Pow(const FixedPoint &x, const FixedPoint &y)
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
    if (x < CONST_ZERO && y.fraction() != 0)
    {
      throw std::runtime_error(
          "Pow(x, y): x^y where x < 0 and y non-integer: mathematical operation not defined!");
    }
    FixedPoint s   = CONST_ONE * ((y.integer() + 1) % 2) + Sign(x) * (y.integer() % 2);
    FixedPoint pow = s * Exp(y * Log(Abs(x)));
    return pow;
  }

  static constexpr FixedPoint Abs(const FixedPoint &x)
  {
    return x * Sign(x);
  }

  static constexpr FixedPoint Sign(const FixedPoint &x)
  {
    return FixedPoint{(x > 0) - (x < 0)};
  }

  static constexpr FixedPoint FromBase(Type n)
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

  constexpr FixedPoint(Type n, const NoScale &)
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
const FixedPoint<I, F> FixedPoint<I, F>::CONST_ZERO{0}; /* 0 */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_ONE{1}; /* 0 */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_SMALLEST_FRACTION(
    0, FixedPoint::SMALLEST_FRACTION); /* 0 */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_E{2.718281828459045235360287471352662498}; /* e */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_LOG2E{
    1.442695040888963407359924681001892137}; /* log_2 e */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_LOG210{3.3219280948874}; /* log_2 10 */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_LOG10E{
    0.434294481903251827651128918916605082}; /* log_10 e */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_LN2{
    0.693147180559945309417232121458176568}; /* log_e 2 */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_LN10{
    2.302585092994045684017991454684364208}; /* log_e 10 */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_PI{3.141592653589793238462643383279502884}; /* pi */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_PI_2{
    1.570796326794896619231321691639751442}; /* pi/2 */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_PI_4{
    0.785398163397448309615660845819875721}; /* pi/4 */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_INV_PI{
    0.318309886183790671537767526745028724}; /* 1/pi */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_2_INV_PI{
    0.636619772367581343075535053490057448}; /* 2/pi */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_2_INV_SQRTPI{
    1.128379167095512573896158903121545172}; /* 2/sqrt(pi) */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_SQRT2{
    1.414213562373095048801688724209698079}; /* sqrt(2) */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::CONST_INV_SQRT2{
    0.707106781186547524400844362104849039}; /* 1/sqrt(2) */
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
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::MAX_EXP =
    FixedPoint::Log(FixedPoint::FromBase(FixedPoint::MAX)); /* maximum exponent for Exp() */
template <std::uint16_t I, std::uint16_t F>
const FixedPoint<I, F> FixedPoint<I, F>::MIN_EXP =
    -FixedPoint::Log(FixedPoint::FromBase(FixedPoint::MAX)); /* minimum exponent for Exp() */

}  // namespace fixed_point
}  // namespace fetch
