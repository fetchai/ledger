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
struct Sin
{
  void operator()(type const &x, type &y) const
  {
    y = std::sin(x);
  }
};

template <typename type>
struct Cos
{
  void operator()(type const &x, type &y) const
  {
    y = std::cos(x);
  }
};

template <typename type>
struct Tan
{
  void operator()(type const &x, type &y) const
  {
    y = std::tan(x);
  }
};

template <typename type>
struct ASin
{
  void operator()(type const &x, type &y) const
  {
    y = std::asin(x);
  }
};

template <typename type>
struct ACos
{
  void operator()(type const &x, type &y) const
  {
    y = std::acos(x);
  }
};

template <typename type>
struct ATan
{
  void operator()(type const &x, type &y) const
  {
    y = std::atan(x);
  }
};

template <typename type>
struct ATan2
{
  void operator()(type const &x, type &y) const
  {
    y = std::atan2(x);
  }
};

template <typename type>
struct SinH
{
  void operator()(type const &x, type &y) const
  {
    y = std::sinh(x);
  }
};

template <typename type>
struct CosH
{
  void operator()(type const &x, type &y) const
  {
    y = std::cosh(x);
  }
};

template <typename type>
struct TanH
{
  void operator()(type const &x, type &y) const
  {
    y = std::tanh(x);
  }
};

template <typename type>
struct ASinH
{
  void operator()(type const &x, type &y) const
  {
    y = std::asinh(x);
  }
};

template <typename type>
struct ACosH
{
  void operator()(type const &x, type &y) const
  {
    y = std::acosh(x);
  }
};

template <typename type>
struct ATanH
{
  void operator()(type const &x, type &y) const
  {
    y = std::atanh(x);
  }
};

}  // namespace kernels
}  // namespace math
}  // namespace fetch
