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

#include <cmath>

namespace fetch {
namespace kernels {

template <typename type, typename vector_register_type>
struct MultiplyScalar
{
  MultiplyScalar(type const &val)
    : scalar(val)
  {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = scalar * x;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct AddScalar
{
  AddScalar(type const &val)
    : scalar(val)
  {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = scalar + x;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct SubtractScalar
{
  SubtractScalar(type const &val)
    : scalar(val)
  {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = x - scalar;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct DivideScalar
{
  DivideScalar(type const &val)
    : scalar(val)
  {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = x / scalar;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct ScalarSubtract
{
  ScalarSubtract(type const &val)
    : scalar(val)
  {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = scalar - x;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct DivideScalar
{
  ScalarDivide(type const &val)
    : scalar(val)
  {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = scalar / x;
  }

  vector_register_type scalar;
};

}  // namespace kernels
}  // namespace fetch
