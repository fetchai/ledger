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

#include "math/base_types.hpp"
#include "math/tensor_slice_iterator.hpp"
#include <cassert>
#include <unordered_set>

namespace fetch {
namespace math {

// need to forward declare
template <typename T, typename C>
class Tensor;

/* Computes the shape resulting from squeezing.
 * @param a is the input shape.
 * @param b is the output shape.
 * @param axis is the axis to squeeze.
 */
inline bool ShapeFromSqueeze(SizeVector const &a, SizeVector &b,
                             SizeType const &axis = SizeType(-1))
{
  SizeType i = 0;
  b.clear();
  if (axis == SizeType(-1))
  {
    for (auto const &v : a)
    {
      if (v != 1)
      {
        b.push_back(v);
      }
      ++i;
    }
  }
  else
  {
    for (auto const &v : a)
    {
      if (!((i == axis) && (v == 1)))
      {
        b.push_back(v);
      }
      ++i;
    }
  }

  return b.size() != a.size();
}

/* Computes the shape resulting from squeezing.
 * @param a is the input shape.
 * @param b is the output shape.
 * @param axes are the axes to squeeze.
 */
inline bool ShapeFromSqueeze(SizeVector const &a, SizeVector &b, SizeSet const &axes)
{
  SizeType i = 0;
  b.clear();
  for (auto const &v : a)
  {
    if (!((axes.find(i) != axes.end()) && (v == 1)))
    {
      b.push_back(v);
    }
    ++i;
  }

  return b.size() != a.size();
}

/* Squeeze an Tensor.
 * @param arr is the array.
 * @param axis is the axes to squeeze.
 */
template <typename T, typename C>
inline void Squeeze(Tensor<T, C> &arr, SizeType const &axis = SizeType(-1))
{
  SizeVector newshape;
  ShapeFromSqueeze(arr.shape(), newshape, axis);
  arr.Reshape(newshape);
}

/* Squeeze an Tensor.
 * @param arr is the array.
 * @param axes are the axes to squeeze.
 */
template <typename T, typename C>
void Squeeze(Tensor<T, C> &arr, SizeSet const &axes)
{
  SizeVector newshape;
  ShapeFromSqueeze(arr.shape(), newshape, axes);
  arr.Reshape(newshape);
}

namespace reduce_details {
template <typename F, typename T, typename C>
inline void Reduce(F fnc, ConstTensorSliceIterator<T, C> &it_a, TensorSliceIterator<T, C> &it_b,
                   SizeType const &N)
{
  while (bool(it_a) && bool(it_b))
  {

    *it_b = *it_a;
    ++it_a;

    for (SizeType i = 0; i < N - 1; ++i)
    {
      *it_b = fnc(*it_b, *it_a);
      ++it_a;
    }
    ++it_b;
  }
}
}  // namespace reduce_details

/* Reduce an Tensor by one dimension.
 * @param fnc is the reduction function.
 * @param input is the array the input array.
 * @param output is the array the output array.
 * @param axis are the axis along which the reduction happens.
 */
template <typename F, typename T, typename C>
inline void Reduce(F fnc, Tensor<T, C> const &input, Tensor<T, C> &output, SizeType const &axis = 0)
{
  SizeType N;

  SizeType   k{1};
  SizeVector out_shape{};
  for (SizeType i = 0; i < input.shape().size(); ++i)
  {
    if (i != axis)
    {
      out_shape.push_back(input.shape(i));
      k *= input.shape(i);
    }
  }
  output.Reshape(out_shape);

  fetch::math::ConstTensorSliceIterator<T, C> it_a(input);
  fetch::math::TensorSliceIterator<T, C>      it_b(output);

  if (axis != 0)
  {
    // Move the axis we want to reduce to the front
    // to make it iterable in the inner most loop.
    it_a.MoveAxisToFront(axis);
  }

  N = it_a.range(0).total_steps;

  reduce_details::Reduce(fnc, it_a, it_b, N);
}

/* Reduce an Tensor by one dimension.
 * @param fnc is the reduction function.
 * @param input is the array the input array.
 * @param output is the array the output array.
 * @param axes are the axes along which the reduction happens.
 */
template <typename F, typename T, typename C>
inline void Reduce(F fnc, Tensor<T, C> const &input, Tensor<T, C> &output, SizeVector const &axes)
{
  SizeType N;

  SizeType   k = 1;
  SizeVector out_shape;
  SizeSet    axes_set(axes.begin(), axes.end());

  for (SizeType i = 0; i < input.shape().size(); ++i)
  {
    if (axes_set.find(i) != axes_set.end())
    {
      out_shape.push_back(input.shape(i));
      k *= input.shape(i);
    }
  }
  output.Reshape(out_shape);

  ConstTensorSliceIterator<T, C> it_a(input);
  TensorSliceIterator<T, C>      it_b(output);

  // Move the axis we want to reduce to the front
  // to make it iterable in the inner most loop.
  it_a.MoveAxisToFront(axes);

  N = 1;
  for (SizeType i = 0; i < axes.size(); ++i)
  {
    N *= it_a.range(i).total_steps;
  }

  reduce_details::Reduce(fnc, it_a, it_b, N);
}

}  // namespace math
}  // namespace fetch
