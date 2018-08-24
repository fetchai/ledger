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

#include "math/ndarray_iterator.hpp"
#include <cassert>
#include <unordered_set>

namespace fetch {
namespace math {

// need to forward declare
template <typename T, typename C>
class NDArray;

/* Computes the shape resulting from squeezing.
 * @param a is the input shape.
 * @param b is the output shape.
 * @param axis is the axis to squeeze.
 */
bool ShapeFromSqueeze(std::vector<std::size_t> const &a, std::vector<std::size_t> &b,
                      uint64_t const &axis = uint64_t(-1))
{
  std::size_t i = 0;
  b.clear();
  if (axis == uint64_t(-1))
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
bool ShapeFromSqueeze(std::vector<std::size_t> const &a, std::vector<std::size_t> &b,
                      std::unordered_set<uint64_t> const &axes)
{
  std::size_t i = 0;
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

/* Squeeze an NDArray.
 * @param arr is the array.
 * @param axis is the axes to squeeze.
 */
template <typename T, typename C>
void Squeeze(NDArray<T, C> &arr, uint64_t const &axis = uint64_t(-1))
{
  std::vector<std::size_t> newshape;
  ShapeFromSqueeze(arr.shape(), newshape, axis);
  arr.LazyReshape(newshape);
}

/* Squeeze an NDArray.
 * @param arr is the array.
 * @param axes are the axes to squeeze.
 */
template <typename T, typename C>
void Squeeze(NDArray<T, C> &arr, std::unordered_set<uint64_t> const &axes)
{
  std::vector<std::size_t> newshape;
  ShapeFromSqueeze(arr.shape(), newshape, axes);
  arr.Reshape(newshape);
}

/* Reduce an NDArray by one dimension.
 * @param fnc is the reduction function.
 * @param input is the array the input array.
 * @param output is the array the output array.
 * @param axis are the axis along which the reduction happens.
 */
template <typename F, typename T, typename C>
void Reduce(F fnc, NDArray<T, C> &input, NDArray<T, C> &output, uint64_t const &axis = 0)
{
  std::size_t N;

  std::size_t              k = 1;
  std::vector<std::size_t> out_shape;
  for (std::size_t i = 0; i < input.shape().size(); ++i)
  {
    if (i != axis)
    {
      out_shape.push_back(input.shape(i));
      k *= input.shape(i);
    }
  }
  output.Resize(k);
  output.Reshape(out_shape);

  NDArrayIterator<T, C> it_a(input);  // TODO(tfr): Make const iterator
  NDArrayIterator<T, C> it_b(output);

  if (axis != 0)
  {
    // Move the axis we want to reduce to the front
    // to make it iterable in the inner most loop.
    it_a.MoveAxesToFront(axis);
  }

  N = it_a.range(0).total_steps;

  while (bool(it_a) && bool(it_b))
  {

    *it_b = *it_a;
    ++it_a;

    for (std::size_t i = 0; i < N - 1; ++i)
    {
      *it_b = fnc(*it_b, *it_a);
      ++it_a;
    }
    ++it_b;
  }
}

/* Reduce an NDArray by one dimension.
 * @param fnc is the reduction function.
 * @param input is the array the input array.
 * @param output is the array the output array.
 * @param axes are the axes along which the reduction happens.
 */
template <typename F, typename T, typename C>
void Reduce(F fnc, NDArray<T, C> &input, NDArray<T, C> &output, std::vector<uint64_t> const &axes)
{
  std::size_t N;

  std::size_t                  k = 1;
  std::vector<std::size_t>     out_shape;
  std::unordered_set<uint64_t> axes_set(axes.begin(), axes.end());

  for (std::size_t i = 0; i < input.shape().size(); ++i)
  {
    if (axes_set.find(i) != axes_set.end())
    {
      out_shape.push_back(input.shape(i));
      k *= input.shape(i);
    }
  }
  output.Resize(k);
  output.Reshape(out_shape);

  NDArrayIterator<T, C> it_a(input);  // TODO(private issue 187): Make const iterator
  NDArrayIterator<T, C> it_b(output);

  // Move the axis we want to reduce to the front
  // to make it iterable in the inner most loop.
  it_a.MoveAxesToFront(axes);

  N = 1;
  for (std::size_t i = 0; i < axes.size(); ++i)
  {
    N *= it_a.range(i).total_steps;
  }

  while (bool(it_a) && bool(it_b))
  {

    *it_b = *it_a;
    ++it_a;

    for (std::size_t i = 0; i < N - 1; ++i)
    {
      *it_b = fnc(*it_b, *it_a);
      ++it_a;
    }
    ++it_b;
  }
}

}  // namespace math
}  // namespace fetch
