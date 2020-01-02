#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/tensor/tensor.hpp"
#include "meta/is_iterable.hpp"

// TODO(1154): move to math library when implementing isiterable methods

namespace fetch {
namespace math {

template <typename T1, typename T2>
fetch::meta::IsIterableTwoArg<T1, T2, void> PolyfillInlineAdd(T1 &ret, T2 const &other)
{
  // To vectorise this operation, we add column by column
  // as the framework garantuees continous aligned segments
  // of memory along the columns
  memory::Range range(0, std::size_t(ret.height()));
  for (uint64_t j = 0; j < ret.width(); ++j)
  {
    auto ret_slice = ret.data().slice(ret.padded_height() * j, ret.padded_height());
    auto slice     = other.data().slice(other.padded_height() * j, other.padded_height());

    ret_slice.in_parallel().RangedApplyMultiple(
        range, [](auto const &a, auto const &b, auto &c) { c = b + a; }, ret_slice, slice);
  }
}

template <typename T1, typename T2>
fetch::meta::IsIterableTwoArg<T1, T2, void> Assign(T1 ret, T2 const &other)
{
  memory::Range range(0, std::size_t(ret.height()));

  // To vectorise this operation, we assign column by column
  // as the framework guarantees continuous aligned segments
  // of memory along the columns
  for (uint64_t j = 0; j < ret.width(); ++j)
  {

    auto ret_slice = ret.data().slice(ret.padded_height() * j, ret.padded_height());
    auto slice     = other.data().slice(other.padded_height() * j, other.padded_height());

    ret_slice.in_parallel().RangedApplyMultiple(range, [](auto const &a, auto &b) { b = a; },
                                                slice);
  }
}

template <typename T1, typename T2>
fetch::meta::IsIterableTwoArg<T1, T2, void> AssignVector(T1 ret, T2 const &other)
{
  ret.data().in_parallel().ApplyMultiple([](auto const &a, auto &b) { b = a; }, other.data());
}

}  // namespace math
}  // namespace fetch
