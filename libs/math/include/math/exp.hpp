#pragma once

#include "core/assert.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>
namespace fetch {
namespace math {

template <uint8_t N, uint64_t C = 60801, bool O = false>
class Exp
{
  // Only using the upper part of a double.
  enum
  {
    E_MANTISSA      = 20,
    E_SIGN          = 1,
    E_EXPONENT      = 11,
    E_ENTRIES       = 1ull << N,
    E_BIN_SIZE      = E_MANTISSA - N,
    E_LITTLE_ENDIAN = 1  // FIXME: Make compile time test
  };

  static constexpr double multiplier_pow2_ = double(1ull << E_MANTISSA);
  static constexpr double exponent_offset_ = ((1ull << (E_EXPONENT - 1)) - 1);

public:
  Exp(Exp const &other) = delete;
  Exp operator=(Exp const &other) = delete;

  Exp() { CreateCorrectionTable(); }

  template <typename T>
  double operator()(T const &x) const
  {
    if (N > E_MANTISSA) return exp(x);
    double in = x * a_ + b_;

    if (O)
    {  // Whether handling overflow or not
      // FIXME: compute max
      //      if(in < x_min_) return 0.0;
      //      if(x > x_max_) return std::numeric_limits< double >::max();
      TODO_FAIL("part not implemented");
    }

    union
    {
      double   d;
      uint32_t i[2];
    } conv;

    conv.i[E_LITTLE_ENDIAN]     = static_cast<uint32_t>(in);
    conv.i[1 - E_LITTLE_ENDIAN] = 0;

    std::size_t c = (conv.i[E_LITTLE_ENDIAN] >> E_BIN_SIZE) & (E_ENTRIES - 1);

    return conv.d * corrections_[c];
  }

  void SetCoefficient(double const &c) { a_ = c * multiplier_pow2_ / M_LN2; }

private:
  double a_ = multiplier_pow2_ / M_LN2;
  double b_ = exponent_offset_ * multiplier_pow2_ - C;

  static double corrections_[E_ENTRIES];
  static bool   initialized_;

  void CreateCorrectionTable()
  {
    if (initialized_) return;

    Exp<0, C>           fexp;
    std::vector<double> accumulated, frequency;
    accumulated.resize(E_ENTRIES);
    frequency.resize(E_ENTRIES);

    for (auto &a : accumulated) a = 0;
    for (auto &a : frequency) a = 0;
    for (double l = 0.; l < 5; l += 0.0000001)
    {  // FIXME: set limit
      double r1 = exp(l);
      double r2 = fexp(l);

      double   z    = l * a_ + b_;
      uint64_t v    = static_cast<uint64_t>(z);
      uint64_t idx  = (v >> E_BIN_SIZE) & (E_ENTRIES - 1);
      double   corr = r1 / r2;

      if (corr == corr)
      {  // if not NaN
        ++frequency[idx];
        accumulated[idx] += corr;
      }
    }

    for (std::size_t i = 0; i < E_ENTRIES; ++i) corrections_[i] = accumulated[i] / frequency[i];

    initialized_ = true;
  }
};

template <uint64_t C, bool OF>
class Exp<0, C, OF>
{
  enum
  {
    E_MANTISSA      = 20,
    E_SIGN          = 1,
    E_EXPONENT      = 11,
    E_ENTRIES       = 0,
    E_LITTLE_ENDIAN = 1  // FIXME: Make compile time test
  };

  static constexpr double multiplier_pow2_ = double(1ull << E_MANTISSA);
  static constexpr double exponent_offset_ = ((1ull << (E_EXPONENT - 1)) - 1);
  double                  a_               = multiplier_pow2_ / M_LN2;
  double                  b_               = exponent_offset_ * multiplier_pow2_ - C;

public:
  Exp() {}
  Exp(Exp const &other) = delete;
  Exp operator=(Exp const &other) = delete;

  template <typename T>
  double operator()(T const &x) const
  {
    union
    {
      double   d;
      uint32_t i[2];
    } conv;

    conv.i[E_LITTLE_ENDIAN]     = static_cast<uint32_t>(x * a_ + b_);
    conv.i[1 - E_LITTLE_ENDIAN] = 0;
    return conv.d;
  }

  void SetCoefficient(double const &c) { a_ = c * multiplier_pow2_ / M_LN2; }
};

template <uint8_t N, uint64_t C, bool OF>
bool Exp<N, C, OF>::initialized_ = false;

template <uint8_t N, uint64_t C, bool OF>
double Exp<N, C, OF>::corrections_[E_ENTRIES] = {0};
}  // namespace math
}  // namespace fetch
