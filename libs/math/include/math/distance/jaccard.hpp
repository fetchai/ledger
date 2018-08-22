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

#include "core/assert.hpp"
#include "math/correlation/jaccard.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type Jaccard(memory::VectorSlice<T, S> const &a,
                                                        memory::VectorSlice<T, S> const &b)
{
  using type = typename memory::VectorSlice<T, S>::type;

  return type(1) - correlation::Jaccard(a, b);
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type Jaccard(ShapeLessArray<T, C> const &a,
                                                   ShapeLessArray<T, C> const &b)
{
  return Jaccard(a.data(), b.data());
}

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type GeneralisedJaccard(
    memory::VectorSlice<T, S> const &a, memory::VectorSlice<T, S> const &b)
{
  using type = typename memory::VectorSlice<T, S>::type;
  return type(1) - correlation::GeneralisedJaccard(a, b);
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type GeneralisedJaccard(ShapeLessArray<T, C> const &a,
                                                              ShapeLessArray<T, C> const &b)
{
  return GeneralisedJaccard(a.data(), b.data());
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
