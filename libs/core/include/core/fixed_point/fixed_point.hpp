#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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


#include <iostream>
//#include <ostream>
//#include <exception>
//#include <cstddef>      // std::size_t
//#include <cstdint>
//#include <type_traits>

#include<cassert>
#include "meta/type_traits.hpp"


namespace fetch {
namespace fixed_point {


template <std::size_t I, std::size_t F>
class FixedPoint;

namespace details {

// struct for inferring
template <std::size_t T>
struct type_from_size
{
  static const bool is_valid = false; // for template matches specialisation
  using value_type = void;
};
//
//
//#if defined(__GNUC__) && defined(__x86_64__)
//template <>
//struct type_from_size<128> {
//  static const bool           is_valid = true;
//  static const std::size_t    size = 128;
//  using value_type = __int128;
//  using unsigned_type = unsigned __int128;
//  using signed_type = __int128;
//  using next_size = type_from_size<256>;
//};
//#endif

// 64 bit implementation
template <>
struct type_from_size<64>
{
  static const bool           is_valid = true;
  static const std::size_t    size = 64;
  using value_type = int64_t;
  using unsigned_type = uint64_t;
  using signed_type = int64_t;
  using next_size = type_from_size<128>;
};

// 32 bit implementation
template <>
struct type_from_size<32>
{
  static const bool          is_valid = true;
  static const std::size_t   size = 32;
  using value_type = int32_t;
  using unsigned_type = uint32_t;
  using signed_type = int32_t;
  using next_size = type_from_size<64>;
};

// 16 bit implementation
template <>
struct type_from_size<16>
{
  static const bool          is_valid = true;
  static const std::size_t   size = 16;
  using value_type = int16_t;
  using unsigned_type = uint16_t;
  using signed_type = int16_t;
  using next_size = type_from_size<32>;
};

// 8 bit implementation
template <>
struct type_from_size<8>
{
  static const bool          is_valid = true;
  static const std::size_t   size = 8;
  using value_type = int8_t;
  using unsigned_type = uint8_t;
  using signed_type = int8_t;
  using next_size = type_from_size<16>;
};

// this is to assist in adding support for non-native base
// types (for adding big-int support), this should be fine
// unless your bit-int class doesn't nicely support casting
template <class B, class N>
B next_to_base(const N& rhs) {
  return static_cast<B>(rhs);
}


template <std::size_t I, std::size_t F>
FixedPoint<I,F> divide(const FixedPoint<I,F> &numerator, const FixedPoint<I,F> &denominator, FixedPoint<I,F> &remainder, typename std::enable_if<type_from_size<I+F>::next_size::is_valid>::type* = 0) {

  typedef typename FixedPoint<I,F>::next_type next_type;
  typedef typename FixedPoint<I,F>::base_type base_type;
  static const std::size_t fractional_bits = FixedPoint<I,F>::fractional_bits;

  next_type t(numerator.to_raw());
  t <<= fractional_bits;

  FixedPoint<I,F> quotient;

  quotient  = FixedPoint<I,F>::from_base(next_to_base<base_type>(t / denominator.to_raw()));
  remainder = FixedPoint<I,F>::from_base(next_to_base<base_type>(t % denominator.to_raw()));

  return quotient;
}

template <std::size_t I, std::size_t F>
FixedPoint<I,F> divide(FixedPoint<I,F> numerator, FixedPoint<I,F> denominator, FixedPoint<I,F> &remainder, typename std::enable_if<!type_from_size<I+F>::next_size::is_valid>::type* = 0) {

  // NOTE(eteran): division is broken for large types :-(
  // especially when dealing with negative quantities

  typedef typename FixedPoint<I,F>::base_type     base_type;
  typedef typename FixedPoint<I,F>::unsigned_type unsigned_type;

  static const int bits = FixedPoint<I,F>::total_bits;

  if(denominator == 0)
  {
    std::exception(); // can't divide by zero
  }
  else
  {

    int sign = 0;

    FixedPoint<I,F> quotient;

    if(numerator < 0) {
      sign ^= 1;
      numerator = -numerator;
    }

    if(denominator < 0) {
      sign ^= 1;
      denominator = -denominator;
    }

    base_type n      = numerator.to_raw();
    base_type d      = denominator.to_raw();
    base_type x      = 1;
    base_type answer = 0;

    // egyptian division algorithm
    while((n >= d) && (((d >> (bits - 1)) & 1) == 0)) {
      x <<= 1;
      d <<= 1;
    }

    while(x != 0) {
      if(n >= d) {
        n      -= d;
        answer += x;
      }

      x >>= 1;
      d >>= 1;
    }

    unsigned_type l1 = n;
    unsigned_type l2 = denominator.to_raw();

    // calculate the lower bits (needs to be unsigned)
    // unfortunately for many fractions this overflows the type still :-/
    const unsigned_type lo = (static_cast<unsigned_type>(n) << F) / denominator.to_raw();

    quotient  = FixedPoint<I,F>::from_base((answer << F) | lo);
    remainder = n;

    if(sign) {
      quotient = -quotient;
    }

    return quotient;
  }
}

// this is the usual implementation of multiplication
template <std::size_t I, std::size_t F>
void multiply(const FixedPoint<I,F> &lhs, const FixedPoint<I,F> &rhs, FixedPoint<I,F> &result, typename std::enable_if<type_from_size<I+F>::next_size::is_valid>::type* = 0) {

  typedef typename FixedPoint<I,F>::next_type next_type;
  typedef typename FixedPoint<I,F>::base_type base_type;

  static const std::size_t fractional_bits = FixedPoint<I,F>::fractional_bits;

  next_type t(static_cast<next_type>(lhs.to_raw()) * static_cast<next_type>(rhs.to_raw()));
  t >>= fractional_bits;
  result = FixedPoint<I,F>::from_base(next_to_base<base_type>(t));
}

// this is the fall back version we use when we don't have a next size
// it is slightly slower, but is more robust since it doesn't
// require and upgraded type
template <std::size_t I, std::size_t F>
void multiply(const FixedPoint<I,F> &lhs, const FixedPoint<I,F> &rhs, FixedPoint<I,F> &result, typename std::enable_if<!type_from_size<I+F>::next_size::is_valid>::type* = 0) {

  typedef typename FixedPoint<I,F>::base_type base_type;

  static const std::size_t fractional_bits = FixedPoint<I,F>::fractional_bits;
  static const std::size_t integer_mask    = FixedPoint<I,F>::integer_mask;
  static const std::size_t fractional_mask = FixedPoint<I,F>::fractional_mask;

  // more costly but doesn't need a larger type
  const base_type a_hi = (lhs.to_raw() & integer_mask) >> fractional_bits;
  const base_type b_hi = (rhs.to_raw() & integer_mask) >> fractional_bits;
  const base_type a_lo = (lhs.to_raw() & fractional_mask);
  const base_type b_lo = (rhs.to_raw() & fractional_mask);

  const base_type x1 = a_hi * b_hi;
  const base_type x2 = a_hi * b_lo;
  const base_type x3 = a_lo * b_hi;
  const base_type x4 = a_lo * b_lo;

  result = FixedPoint<I,F>::from_base((x1 << fractional_bits) + (x3 + x2) + (x4 >> fractional_bits));

}

















/**
 * finds most significant set bit in type
 * @tparam T
 * @param n
 * @return
 */
template <typename T>
std::size_t HighestSetBit(T n)
{
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
  if (HighestSetBit(n) + fractional_bits <= total_bits)
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
 * @param fractional_bits
 * @param total_bits
 * @return
 */
template <typename T>
bool CheckNoRounding(T n, std::size_t fractional_bits, std::size_t total_bits)
{
  // Not yet implemented
  assert(0);
}



} // namespace details


template <std::size_t I, std::size_t F>
class FixedPoint
{
  static_assert(details::type_from_size<I + F>::is_valid, "invalid combination of sizes");

public:
  static const std::size_t fractional_bits = F;
  static const std::size_t integer_bits    = I;
  static const std::size_t total_bits      = I + F;



  using base_type_info = details::type_from_size<total_bits>;

  using base_type = typename base_type_info::value_type;
  using next_type = typename base_type_info::next_size::value_type;
  using unsigned_type = typename base_type_info::unsigned_type;

  const std::size_t base_size = base_type_info::size;
  const base_type fractional_mask = unsigned_type((2^fractional_bits) - 1);
  const base_type integer_mask    = ~fractional_mask;

  static const base_type one = base_type(1) << fractional_bits;

private:
  base_type data_; // the value to be stored

public:


  ////////////////////
  /// constructors ///
  ////////////////////

  FixedPoint() : data_(0) {}  // initialise to zero

  // tell compiler not to worry about loss of precision, we explicitly check
  // with assert statements
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wconversion"

  template <typename T>
  explicit FixedPoint(T n, meta::IfIsInteger <T> * = nullptr) : data_(static_cast<base_type>(n) << static_cast<base_type>(fractional_bits))
  {
    assert(details::CheckNoOverflow(n, fractional_bits, total_bits));
  }

//
//  FixedPoint(float n) : data_(static_cast<base_type>(n * one)) {
//    assert(details::CheckNoOverflow(n, fractional_bits, total_bits));
//    assert(details::CheckNoRounding(n, fractional_bits, total_bits));
//  }
//  FixedPoint(double n) : data_(static_cast<base_type>(n * one))  {
//    assert(details::CheckNoOverflow(n, fractional_bits, total_bits));
//    assert(details::CheckNoRounding(n, fractional_bits, total_bits));
//  }
  FixedPoint(const FixedPoint &o) : data_(o.data_) {
  }

  #pragma GCC diagnostic pop

  /////////////////
  /// operators ///
  /////////////////

  FixedPoint& operator=(const FixedPoint &o) {
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

  FixedPoint& operator++()
  {
    data_ += one;
    return *this;
  }

  FixedPoint& operator--()
  {
    data_ -= one;
    return *this;
  }

  //////////////////////
  /// math operators ///
  //////////////////////


  FixedPoint operator+(const FixedPoint &n) const
  {
    return (data_ + n.data_);
  }

  template <typename T>
  FixedPoint operator+(const T &n) const
  {
    return FixedPoint(T(data_) + n);
  }

  FixedPoint operator-(const FixedPoint &n) const
  {
    return (data_ - n.data_);
  }

  FixedPoint& operator+=(const FixedPoint &n)
  {
    data_ += n.data_;
    return *this;
  }

  FixedPoint& operator-=(const FixedPoint &n)
  {
    data_ -= n.data_;
    return *this;
  }

  FixedPoint& operator&=(const FixedPoint &n)
  {
    data_ &= n.data_;
    return *this;
  }

  FixedPoint& operator|=(const FixedPoint &n)
  {
    data_ |= n.data_;
    return *this;
  }

  FixedPoint& operator^=(const FixedPoint &n)
  {
    data_ ^= n.data_;
    return *this;
  }

  FixedPoint& operator*=(const FixedPoint &n)
  {
    details::multiply(*this, n, *this);
    return *this;
  }

  FixedPoint& operator/=(const FixedPoint &n)
  {
    FixedPoint temp;
    *this = details::divide(*this, n, temp);
    return *this;
  }

  FixedPoint& operator>>=(const FixedPoint &n)
  {
    data_ >>= n.to_int();
    return *this;
  }

  FixedPoint& operator<<=(const FixedPoint &n)
  {
    data_ <<= n.to_int();
    return *this;
  }

  /////////////////////////////////
  /// conversion to basic types ///
  /////////////////////////////////

  int to_int() const
  {
    return (data_ & integer_mask) >> fractional_bits;
  }

  unsigned int to_uint() const
  {
    return (data_ & integer_mask) >> fractional_bits;
  }

  float to_float() const
  {
    return static_cast<float>(data_) / FixedPoint::one;
  }

  double to_double() const
  {
    return static_cast<double>(data_) / FixedPoint::one;
  }

  base_type to_raw() const
  {
    return data_;
  }


  ////////////
  /// swap ///
  ////////////

  void swap(FixedPoint &rhs)
  {
    std::swap(data_, rhs.data_);
  }


private:
  // this makes it simpler to create a fixed point object from
  // a native type without scaling
  // use "FixedPoint::from_base" in order to perform this.
  struct NoScale {};

  FixedPoint(base_type n, const NoScale &) : data_(n) {
  }

public:
  static FixedPoint from_base(base_type n) {
    return FixedPoint(n, NoScale());
  }

};



} // namespace fixed_point
} // namespace fetch









//
//
//// if we have the same fractional portion, but differing integer portions, we trivially upgrade the smaller type
//template <std::size_t I1, std::size_t I2, std::size_t F>
//typename std::conditional<I1 >= I2, FixedPoint<I1,F>, FixedPoint<I2,F>>::type operator+(const FixedPoint<I1,F> &lhs, const FixedPoint<I2,F> &rhs) {
//
//  typedef typename std::conditional<
//    I1 >= I2,
//    FixedPoint<I1,F>,
//    FixedPoint<I2,F>
//  >::type T;
//
//  const T l = T::from_base(lhs.to_raw());
//  const T r = T::from_base(rhs.to_raw());
//  return l + r;
//}
//
//template <std::size_t I1, std::size_t I2, std::size_t F>
//typename std::conditional<I1 >= I2, FixedPoint<I1,F>, FixedPoint<I2,F>>::type operator-(const FixedPoint<I1,F> &lhs, const FixedPoint<I2,F> &rhs) {
//
//  typedef typename std::conditional<
//    I1 >= I2,
//    FixedPoint<I1,F>,
//    FixedPoint<I2,F>
//  >::type T;
//
//  const T l = T::from_base(lhs.to_raw());
//  const T r = T::from_base(rhs.to_raw());
//  return l - r;
//}
//
//template <std::size_t I1, std::size_t I2, std::size_t F>
//typename std::conditional<I1 >= I2, FixedPoint<I1,F>, FixedPoint<I2,F>>::type operator*(const FixedPoint<I1,F> &lhs, const FixedPoint<I2,F> &rhs) {
//
//  typedef typename std::conditional<
//    I1 >= I2,
//    FixedPoint<I1,F>,
//    FixedPoint<I2,F>
//  >::type T;
//
//  const T l = T::from_base(lhs.to_raw());
//  const T r = T::from_base(rhs.to_raw());
//  return l * r;
//}
//
//template <std::size_t I1, std::size_t I2, std::size_t F>
//typename std::conditional<I1 >= I2, FixedPoint<I1,F>, FixedPoint<I2,F>>::type operator/(const FixedPoint<I1,F> &lhs, const FixedPoint<I2,F> &rhs) {
//
//  typedef typename std::conditional<
//    I1 >= I2,
//    FixedPoint<I1,F>,
//    FixedPoint<I2,F>
//  >::type T;
//
//  const T l = T::from_base(lhs.to_raw());
//  const T r = T::from_base(rhs.to_raw());
//  return l / r;
//}
//
//template <std::size_t I, std::size_t F>
//std::ostream &operator<<(std::ostream &os, const FixedPoint<I,F> &f) {
//  os << f.to_double();
//  return os;
//}
//
//template <std::size_t I, std::size_t F>
//const std::size_t FixedPoint<I,F>::fractional_bits;
//
//template <std::size_t I, std::size_t F>
//const std::size_t FixedPoint<I,F>::integer_bits;
//
//template <std::size_t I, std::size_t F>
//const std::size_t FixedPoint<I,F>::total_bits;
//
//}
//
//#endif