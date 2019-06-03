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

#include "math/tensor.hpp"
#include "meta/is_iterable.hpp"

namespace fetch {
namespace math {

template <typename T1, typename T2>
fetch::meta::IsIterableTwoArg<T1, T2, void> PolyfillInlineAdd(T1 &ret, T2 const &other)
{
  using Type               = typename T1::Type;
  using VectorRegisterType = typename TensorView<Type>::VectorRegisterType;

  // To vectorise this operation, we add column by column
  // as the framework garantuees continous aligned segments 
  // of memory along the columns
  memory::TrivialRange range(0, std::size_t(ret.height()));
  for (uint64_t j = 0; j < ret.width(); ++j)
  {
    auto ret_slice = ret.data().slice(ret.padded_height() * j, ret.padded_height());
    auto slice     = other.data().slice(other.padded_height() * j, other.padded_height());

    ret_slice.in_parallel().Apply(range,
                                  [](VectorRegisterType const &a, VectorRegisterType const &b,
                                     VectorRegisterType &c) { c = b + a; },
                                  ret_slice, slice);
  }
}

template <typename T1, typename T2>
fetch::meta::IsIterableTwoArg<T1, T2, void> Assign(T1 ret, T2 const &other)
{
  using Type               = typename T1::Type;
  using VectorRegisterType = typename TensorView<Type>::VectorRegisterType;

  memory::TrivialRange range(0, std::size_t(ret.height()));

  // To vectorise this operation, we assign column by column
  // as the framework garantuees continous aligned segments 
  // of memory along the columns
  for (uint64_t j = 0; j < ret.width(); ++j)
  {

    auto ret_slice = ret.data().slice(ret.padded_height() * j, ret.padded_height());
    auto slice     = other.data().slice(other.padded_height() * j, other.padded_height());

    ret_slice.in_parallel().Apply(
        range, [](VectorRegisterType const &a, VectorRegisterType &b) { b = a; }, slice);
  }
}

template <typename T1, typename T2>
fetch::meta::IsIterableTwoArg<T1, T2, void> AssignVector(T1 ret, T2 const &other)
{
  using Type               = typename T1::Type;
  using VectorRegisterType = typename TensorView<Type>::VectorRegisterType;

  ret.data().in_parallel().Apply([](VectorRegisterType const &a, VectorRegisterType &b) { b = a; },
                                 other.data());
}

}  // namespace math
}  // namespace fetch