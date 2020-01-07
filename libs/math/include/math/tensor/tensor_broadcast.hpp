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

#include "math/base_types.hpp"
#include "math/tensor/tensor_declaration.hpp"
#include "math/tensor/tensor_slice_iterator.hpp"

#include <cassert>

namespace fetch {
namespace math {

inline bool ShapeFromBroadcast(SizeVector const &a, SizeVector const &b, SizeVector &c)
{
  c.resize(std::max(a.size(), b.size()));

  auto it1 = a.rbegin();
  auto it2 = b.rbegin();
  auto cit = c.rbegin();

  while ((it1 != a.rend()) && (it2 != b.rend()))
  {

    assert(cit != c.rend());

    // dimension is the same in both arrays (i.e. no broadcasting in this dimension)
    if ((*it1) == (*it2))
    {
      (*cit) = *it1;
    }

    // broadcasting in this dimensions
    else
    {

      // not possible to broadcast when dimension sizes differ and neither is 1
      if (((*it1) != 1) && ((*it2) != 1))
      {
        return false;
      }

      *cit = std::max(*it1, *it2);
    }

    ++it1;
    ++it2;
    ++cit;
  }

  while (it1 != a.rend())
  {
    assert(cit != c.rend());
    (*cit) = *it1;
    ++it1;
    ++cit;
  }

  while (it2 != b.rend())
  {
    assert(cit != c.rend());
    (*cit) = *it2;
    ++it2;
    ++cit;
  }

  return true;
}

template <class IteratorType>
bool UpgradeIteratorFromBroadcast(SizeVector const &a, IteratorType &iterator)
{
  assert(iterator.counter() == 0);   // Only upgrade untouched iterators.
  iterator.counter_ = uint64_t(-1);  // Invalidating the iterator

  auto &b   = iterator.ranges_;
  auto  it1 = a.rbegin();
  auto  it2 = b.rbegin();

  while ((it1 != a.rend()) && (it2 != b.rend()))
  {
    if ((it2->total_steps) == 1)
    {
      it2->repeat_dimension = *it1;
      iterator.size_ *= *it1;
    }
    else if ((*it1) != (it2->total_steps))
    {
      return false;
    }

    ++it1;
    ++it2;
  }

  SizeType total_repeats = 1;
  while (it1 != a.rend())
  {
    total_repeats *= (*it1);
    ++it1;
  }

  iterator.total_runs_ = total_repeats;

  iterator.counter_ = 0;

  return true;
}

template <typename T, typename C>
std::vector<SizeVector> PrepareRange(Tensor<T, C> const &a)
{
  std::vector<SizeVector> range;
  for (auto &i : a.shape())
  {
    range.push_back({0, i});
  }
  return range;
}

/**
 * Two inputs Broadcast using given function
 * ret will be reshaped to shape of largest input
 * @tparam F
 * @tparam T
 * @tparam C
 * @param function
 * @param a first input tensor
 * @param b second input tensor
 * @param ret output tensor
 * @return bool true if broadcasting was successful
 */
template <typename F, typename T, typename C>
bool Broadcast(F function, const Tensor<T, C> &a, const Tensor<T, C> &b, Tensor<T, C> &ret)
{
  // Compute output shape
  SizeVector ret_shape;
  ShapeFromBroadcast(a.shape(), b.shape(), ret_shape);
  ret.Reshape(ret_shape);

  // Prepare ranges
  std::vector<SizeVector> a_range   = PrepareRange(a);
  std::vector<SizeVector> b_range   = PrepareRange(b);
  std::vector<SizeVector> ret_range = PrepareRange(ret);

  // Prepare iterators
  ConstTensorSliceIterator<T, C> it_a(a, a_range);
  ConstTensorSliceIterator<T, C> it_b(b, b_range);
  TensorSliceIterator<T, C>      it_ret(ret, ret_range);

  // Change iterators to be broadcasted
  if (!UpgradeIteratorFromBroadcast<ConstTensorSliceIterator<T, C>>(ret_shape, it_a))
  {
    return false;
  }

  if (!UpgradeIteratorFromBroadcast<ConstTensorSliceIterator<T, C>>(ret_shape, it_b))
  {
    return false;
  }

  // Apply function
  while (it_ret)
  {
    function(*it_a, *it_b, *it_ret);

    ++it_a;
    ++it_b;
    ++it_ret;
  }

  return true;
}

/**
 * One input Broadcast using given function
 * Input has to be broadcastable along ret shape
 * ret shape is preserved
 * @tparam F
 * @tparam T
 * @tparam C
 * @param function
 * @param a input tensor
 * @param ret output tensor
 * @return bool true if broadcasting was successful
 */
template <typename F, typename T, typename C>
bool Broadcast(F function, const Tensor<T, C> &a, Tensor<T, C> &ret)
{
  // Prepare ranges
  std::vector<SizeVector> a_range   = PrepareRange(a);
  std::vector<SizeVector> ret_range = PrepareRange(ret);

  // Prepare iterators
  ConstTensorSliceIterator<T, C> it_a(a, a_range);
  TensorSliceIterator<T, C>      it_ret(ret, ret_range);

  // Change iterators to be broadcasted
  if (!UpgradeIteratorFromBroadcast<ConstTensorSliceIterator<T, C>>(ret.shape(), it_a))
  {
    return false;
  }

  // Apply function
  while (it_ret)
  {
    function(*it_a, *it_ret);

    ++it_a;
    ++it_ret;
  }

  return true;
}

}  // namespace math
}  // namespace fetch
