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
#include <cmath>

namespace fetch {
namespace math {
namespace kernels {

template <typename type>
struct Remquo
{
  void operator()(type const &x, type &y) const
  {
    y = std::remquo(x);
  }
};

template <typename type>
struct Fma
{
  void operator()(type const &x, type &y) const
  {
    y = std::fma(x);
  }
};

template <typename type>
struct Fmax
{
  void operator()(type const &x, type &y) const
  {
    y = std::fmax(x);
  }
};

template <typename type>
struct Fmin
{
  void operator()(type const &x, type &y) const
  {
    y = std::fmin(x);
  }
};

template <typename type>
struct Fdim
{
  void operator()(type const &x, type &y) const
  {
    y = std::fdim(x);
  }
};

template <typename type>
struct Nan
{
  void operator()(type const &x, type &y) const
  {
    y = std::nan(x);
  }
};

template <typename type>
struct Nanf
{
  void operator()(type const &x, type &y) const
  {
    y = std::nanf(x);
  }
};

template <typename type>
struct Nanl
{
  void operator()(type const &x, type &y) const
  {
    y = std::nanl(x);
  }
};

template <typename type>
struct Exp2
{
  void operator()(type const &x, type &y) const
  {
    y = std::exp2(x);
  }
};

template <typename type>
struct Expm1
{
  void operator()(type const &x, type &y) const
  {
    y = std::expm1(x);
  }
};

template <typename type>
struct Log
{
  void operator()(type const &x, type &y) const
  {
    y = std::log(x);
  }
};

template <typename type>
struct Log10
{
  void operator()(type const &x, type &y) const
  {
    y = std::log10(x);
  }
};

template <typename type>
struct Log2
{
  void operator()(type const &x, type &y) const
  {
    y = std::log2(x);
  }
};

template <typename type>
struct Log1p
{
  void operator()(type const &x, type &y) const
  {
    y = std::log1p(x);
  }
};

template <typename type>
struct Pow
{
  void operator()(type const &x, type &y) const
  {
    y = std::pow(x);
  }
};

template <typename type>
struct Sqrt
{
  void operator()(type const &x, type &y) const
  {
    y = std::sqrt(x);
  }
};

template <typename type>
struct Cbrt
{
  void operator()(type const &x, type &y) const
  {
    y = std::cbrt(x);
  }
};

template <typename type>
struct Hypot
{
  void operator()(type const &x, type &y) const
  {
    y = std::hypot(x);
  }
};

template <typename type>
struct Erf
{
  void operator()(type const &x, type &y) const
  {
    y = std::erf(x);
  }
};

template <typename type>
struct Erfc
{
  void operator()(type const &x, type &y) const
  {
    y = std::erfc(x);
  }
};

template <typename type>
struct Tgamma
{
  void operator()(type const &x, type &y) const
  {
    y = std::tgamma(x);
  }
};

template <typename type>
struct Lgamma
{
  void operator()(type const &x, type &y) const
  {
    y = std::lgamma(x);
  }
};

template <typename type>
struct Ceil
{
  void operator()(type const &x, type &y) const
  {
    y = std::ceil(x);
  }
};

template <typename type>
struct Floor
{
  void operator()(type const &x, type &y) const
  {
    y = std::floor(x);
  }
};

template <typename type>
struct Trunc
{
  void operator()(type const &x, type &y) const
  {
    y = std::trunc(x);
  }
};

template <typename type>
struct Round
{
  void operator()(type const &x, type &y) const
  {
    y = std::round(x);
  }
};

template <typename type>
struct Lround
{
  void operator()(type const &x, type &y) const
  {
    y = static_cast<type>(std::lround(x));
  }
};

template <typename type>
struct Llround
{
  void operator()(type const &x, type &y) const
  {
    y = static_cast<type>(std::llround(x));
  }
};

template <typename type>
struct Nearbyint
{
  void operator()(type const &x, type &y) const
  {
    y = std::nearbyint(x);
  }
};

template <typename type>
struct Rint
{
  void operator()(type const &x, type &y) const
  {
    y = std::rint(x);
  }
};

template <typename type>
struct Lrint
{
  void operator()(type const &x, type &y) const
  {
    y = static_cast<type>(std::lrint(x));
  }
};

template <typename type>
struct Llrint
{
  void operator()(type const &x, type &y) const
  {
    y = static_cast<type>(std::llrint(x));
  }
};

template <typename type>
struct Frexp
{
  void operator()(type const &x, type &y) const
  {
    y = std::frexp(x);
  }
};

template <typename type>
struct Ldexp
{
  void operator()(type const &x, type &y) const
  {
    y = std::ldexp(x);
  }
};

template <typename type>
struct Modf
{
  void operator()(type const &x, type &y) const
  {
    y = std::modf(x);
  }
};

template <typename type>
struct Scalbn
{
  void operator()(type const &x, type &y) const
  {
    y = std::scalbn(x);
  }
};

template <typename type>
struct Scalbln
{
  void operator()(type const &x, type &y) const
  {
    y = std::scalbln(x);
  }
};

template <typename type>
struct Ilogb
{
  void operator()(type const &x, type &y) const
  {
    y = std::ilogb(x);
  }
};

template <typename type>
struct Logb
{
  void operator()(type const &x, type &y) const
  {
    y = std::logb(x);
  }
};

template <typename type>
struct Nextafter
{
  void operator()(type const &x, type &y) const
  {
    y = std::nextafter(x);
  }
};

template <typename type>
struct Nexttoward
{
  void operator()(type const &x, type &y) const
  {
    y = std::nexttoward(x);
  }
};

template <typename type>
struct Copysign
{
  void operator()(type const &x, type &y) const
  {
    y = std::copysign(x);
  }
};

template <typename type>
struct Fpclassify
{
  void operator()(type const &x, type &y) const
  {
    y = std::fpclassify(x);
  }
};

template <typename type>
struct Isfinite
{
  void operator()(type const &x, type &y) const
  {
    y = std::isfinite(x);
  }
};

template <typename type>
struct Isinf
{
  void operator()(type const &x, type &y) const
  {
    y = std::isinf(x);
  }
};

template <typename type>
struct Isnan
{
  void operator()(type const &x, type &y) const
  {
    y = std::isnan(x);
  }
};

template <typename type>
struct Isnormal
{
  void operator()(type const &x, type &y) const
  {
    y = std::isnormal(x);
  }
};

template <typename type>
struct Signbit
{
  void operator()(type const &x, type &y) const
  {
    y = std::signbit(x);
  }
};

template <typename type>
struct Isgreater
{
  void operator()(type const &x, type &y) const
  {
    y = std::isgreater(x);
  }
};

template <typename type>
struct Isgreaterequal
{
  void operator()(type const &x, type const &y, type &z) const
  {
    z = std::isgreaterequal(x, y);
  }
};

template <typename type>
struct Isless
{
  void operator()(type const &x, type &y) const
  {
    y = std::isless(x);
  }
};

template <typename type>
struct Islessequal
{
  void operator()(type const &x, type &y) const
  {
    y = std::islessequal(x);
  }
};

template <typename type>
struct Islessgreater
{
  void operator()(type const &x, type &y) const
  {
    y = std::islessgreater(x);
  }
};

template <typename type>
struct Isunordered
{
  void operator()(type const &x, type &y) const
  {
    y = std::isunordered(x);
  }
};

}  // namespace kernels
}  // namespace math
}  // namespace fetch
