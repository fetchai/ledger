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

#include <cassert>
#include <numeric>

#include "core/assert.hpp"

#include "vectorise/memory/range.hpp"

#include "math/free_functions/comparison/comparison.hpp"
#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/kernels/basic_arithmetics.hpp"
#include "math/kernels/standard_functions.hpp"
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapelessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

template <typename T, typename C>
T Max(ShapelessArray<T, C> const &array);

/**
 * calculates bit mask on this
 * @param x
 */
namespace details {
template <typename ArrayType>
void BooleanMaskImplementation(ArrayType &input_array, ArrayType const &mask, ArrayType &ret)
{
  assert(input_array.size() == mask.size());

  std::size_t counter = 0;
  for (std::size_t i = 0; i < input_array.size(); ++i)
  {
    assert((mask[i] == 1) || (mask[i] == 0));
    // TODO(private issue 193): implement boolean only ndarray to avoid cast
    if (bool(mask[i]))
    {
      ret[counter] = input_array[i];
      ++counter;
    }
  }

  ret.LazyResize(counter);
}
}  // namespace details
template <typename T, typename C>
void BooleanMask(ShapelessArray<T, C> &input_array, ShapelessArray<T, C> const &mask,
                 ShapelessArray<T, C> &ret)
{
  details::BooleanMaskImplementation(input_array, mask, ret);
}
template <typename T, typename C>
ShapelessArray<T, C> BooleanMask(ShapelessArray<T, C> &      input_array,
                                 ShapelessArray<T, C> const &mask)
{
  ShapelessArray<T, C> ret;
  BooleanMask(input_array, mask, ret);
  return ret;
}
template <typename T, typename C>
void BooleanMask(NDArray<T, C> &input_array, NDArray<T, C> &mask, NDArray<T, C> &ret)
{
  assert(input_array.shape().size() >= mask.shape().size());
  assert(mask.shape().size() > 0);

  // because tensorflow is row major by default - we have to flip the mask and array to get the same
  // answer
  // TODO(private issue 208)
  input_array.MajorOrderFlip();
  mask.MajorOrderFlip();

  if (mask.shape() == input_array.shape())
  {
    details::BooleanMaskImplementation(input_array, mask, ret);
  }
  else
  {
    for (std::size_t j = 0; j < mask.shape().size(); ++j)
    {
      assert(mask.shape()[j] == input_array.shape()[j]);
    }

    // new shape should be n-k+1 dimensions
    std::vector<std::size_t> new_shape;
    NDArray<T, C>            ret{new_shape};

    // TODO(private issue 207): perhaps a little bit hacky to implement boolean mask as a
    // multiplication
    Broadcast([](T x, T y) { return x * y; }, input_array, mask, ret);
  }
}
template <typename T, typename C>
NDArray<T, C> BooleanMask(NDArray<T, C> &input_array, NDArray<T, C> &mask)
{
  NDArray<T, C> ret;
  BooleanMask(input_array, mask, ret);
  return ret;
}

/*
 * Adds sparse updates to the variable referenced
 */
namespace details {
template <typename ArrayType>
void ScatterImplementation(ArrayType &input_array, ArrayType &updates, ArrayType &indices)
{
  // sort indices and updates into ascending order
  std::vector<std::pair<std::size_t, typename ArrayType::Type>> AB;

  // copy into pairs
  // Note that A values are put in "first" this is very important
  for (std::size_t i = 0; i < updates.size(); ++i)
  {
    AB.push_back(std::make_pair(indices[i], updates[i]));
  }

  std::sort(AB.begin(), AB.end());

  // Place back into arrays
  for (size_t i = 0; i < updates.size(); ++i)
  {
    updates[i] = AB[i].second;
    indices[i] = static_cast<typename ArrayType::type>(AB[i].first);
  }

  // scatter
  std::size_t arr_count = 0;
  for (std::size_t count = 0; count < indices.size(); ++count)
  {
    // TODO(private issue 282): Think about this code
    while (arr_count < static_cast<std::size_t>(indices[count]))
    {
      ++arr_count;
    }

    input_array[arr_count] = updates[count];
  }
}
}  // namespace details

/**
 * Copies the values of updates into the specified indices of the first dimension of data in this
 * object
 */
template <typename T, typename C>
void Scatter(ShapelessArray<T, C> &input_array, ShapelessArray<T, C> const &updates,
             ShapelessArray<T, C> const &indices)
{
  details::ScatterImplementation(input_array, updates, indices);
}

template <typename T, typename C>
void Scatter(NDArray<T, C> &input_array, NDArray<T, C> &updates, NDArray<T, C> &indices)
{

  assert(input_array.size() >= updates.size());
  assert(updates.shape().size() > 0);
  assert(input_array.size() >= updates.size());

  // because tensorflow is row major by default we have to flip to get the same answer
  // TODO(private issue 208)
  input_array.MajorOrderFlip();
  updates.MajorOrderFlip();

  details::ScatterImplementation(input_array, updates, indices);
}

/**
 * gathers data from first dimension of data, according to indices, and puts them into input array
 * self_type
 */
template <typename T, typename C>
void Gather(NDArray<T, C> &input_array, NDArray<T, C> &updates, NDArray<T, C> &indices)
{
  assert(input_array.size() >= updates.size());
  assert(updates.size() > 0);
  input_array.LazyReshape(updates.shape());

  if (input_array.shape().size() > 1)
  {
    input_array.MajorOrderFlip();
  }
  if (input_array.shape().size() > 1)
  {
    updates.MajorOrderFlip();
  }

  input_array.LazyResize(indices.size());
  input_array.LazyReshape(indices.shape());

  // sort indices
  indices.Sort();

  // set up an iterator
  NDArrayIterator<T, C> arr_iterator{updates};
  NDArrayIterator<T, C> ret_iterator{input_array};

  std::size_t cur_idx, arr_count = 0;
  for (std::size_t count = 0; count < indices.size(); ++count)
  {
    cur_idx = std::size_t(indices[count]);

    while (arr_count < cur_idx)
    {
      ++arr_iterator;
      ++arr_count;
    }

    *ret_iterator = *arr_iterator;
    ++ret_iterator;
  }
}

/**
 * Finds the maximum value in an array
 * @tparam T data type
 * @tparam C container type
 * @param array array to search for maximum value
 * @return
 */
template <typename T, typename C>
T Max(ShapelessArray<T, C> const &array, T &ret)
{
  using vector_register_type = typename ShapelessArray<T, C>::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
  return ret;
}
template <typename T, typename C>
T Max(ShapelessArray<T, C> const &array)
{
  T ret;
  Max(array, ret);
  return ret;
}

/**
 * Finds the maximum value in a range of the array
 * @tparam T
 * @tparam C
 * @param array
 * @param r
 * @return
 */
template <typename T, typename C>
inline void Max(ShapelessArray<T, C> const &array, memory::Range r, T &ret)
{
  using vector_register_type = typename ShapelessArray<T, C>::vector_register_type;

  if (r.is_trivial())
  {
    ret = array.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
  }
  else
  {  // non-trivial range is not vectorised
    typename ShapelessArray<T, C>::Type ret =
        -std::numeric_limits<typename ShapelessArray<T, C>::Type>::max();
    for (auto i : array)
    {
      ret = std::max(ret, i);
    }
  }
}

/**
 * Implementation of Max that returns the n-1 dim array by finding the max of all 1-d vectors within
 * the array
 * @tparam T
 * @tparam C
 * @param array
 * @param axis
 * @param ret
 */
template <typename T, typename C>
void Max(NDArray<T, C> &array, std::size_t const &axis, NDArray<T, C> &ret)
{
  assert(axis < array.shape().size());

  NDArrayIterator<T, C> return_iterator{ret};

  // iterate through the return array (i.e. the array of Max vals)
  std::vector<std::size_t> cur_index;
  while (return_iterator)
  {
    std::vector<std::vector<std::size_t>> cur_step;

    cur_index = return_iterator.GetNDimIndex();

    // calculate which part of the array we should iterate over (i.e. identify the 1-d vectors
    // within the array)
    std::size_t index_counter = 0;
    for (std::size_t i = 0; i < array.shape().size(); ++i)
    {
      if (i == axis)
      {
        cur_step.push_back({0, array.shape()[i]});
      }
      else
      {
        cur_step.push_back({cur_index[index_counter], cur_index[index_counter] + 1});
        ++index_counter;
      }
    }

    // get an iterator to iterate over the 1-d slice of the array to calculate max over
    NDArrayIterator<T, C> array_iterator(array, cur_step);

    // loops through the 1d array calculating the max val
    typename NDArray<T, C>::Type cur_max =
        -std::numeric_limits<typename NDArray<T, C>::Type>::max();
    typename NDArray<T, C>::Type cur_val;
    while (array_iterator)
    {
      cur_val = *array_iterator;
      Max(cur_max, cur_val, cur_max);
      ++array_iterator;
    }

    *return_iterator = cur_max;
    ++return_iterator;
  }
}

/**
 * Min function for returning the smallest value in an array
 * @tparam ArrayType
 * @param array
 * @return
 */
template <typename T, typename C>
inline void Min(ShapelessArray<T, C> const &array, T &ret)
{
  using vector_register_type = typename ShapelessArray<T, C>::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });
}

/**
 * Finds the minimum value in a range of the array
 * @tparam T
 * @tparam C
 * @param array
 * @param r
 * @return
 */
template <typename T, typename C>
inline void Min(ShapelessArray<T, C> const &array, memory::Range r, T &ret)
{
  using vector_register_type = typename ShapelessArray<T, C>::vector_register_type;

  if (r.is_trivial())
  {
    ret = array.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });
  }
  else
  {  // non-trivial range is not vectorised
    typename T::Type ret = std::numeric_limits<typename T::Type>::max();
    for (auto i : array)
    {
      ret = std::min(ret, i);
    }
  }
}

/**
 * find the minimum of the 1-D projections through the array
 */
template <typename T, typename C>
void Min(NDArray<T, C> &array, std::size_t const &axis, NDArray<T, C> &ret)
{
  assert(axis < array.shape().size());

  NDArrayIterator<T, C> return_iterator{ret};

  // iterate through the return array (i.e. the array of Max vals)
  //    type cur_val;
  std::vector<std::size_t> cur_index;
  while (return_iterator)
  {
    std::vector<std::vector<std::size_t>> cur_step;

    cur_index = return_iterator.GetNDimIndex();

    // calculate step from cur_index and axis
    std::size_t index_counter = 0;
    for (std::size_t i = 0; i < array.shape().size(); ++i)
    {
      if (i == axis)
      {
        cur_step.push_back({0, array.shape()[i]});
      }
      else
      {
        cur_step.push_back({cur_index[index_counter], cur_index[index_counter] + 1});
        ++index_counter;
      }
    }

    // get an iterator to iterate over the 1-d slice of the array to calculate max over
    NDArrayIterator<T, C> array_iterator(array, cur_step);

    // loops through the 1d array calculating the max val
    T cur_max = std::numeric_limits<T>::max();
    T cur_val;
    while (array_iterator)
    {
      cur_val = *array_iterator;
      Min(cur_max, cur_val, cur_max);
      ++array_iterator;
    }

    *return_iterator = cur_max;
    ++return_iterator;
  }
}

/**
 * Returns an array containing the elementwise maximum of two other ndarrays
 * @param x array input 1
 * @param y array input 2
 * @return the combined array
 */
namespace details {
template <typename ArrayType>
ArrayType &MaximumImplementation(ArrayType const &array1, ArrayType const &array2, ArrayType &ret)
{
  assert(array1.size() == array2.size());
  assert(ret.size() == array2.size());

  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = std::max(array1[i], array2[i]);
  }
  return ret;
}
}  // namespace details
template <typename T, typename C>
void Maximum(NDArray<T, C> const &array1, NDArray<T, C> const &array2, NDArray<T, C> &ret)
{
  assert(ret.shape() == array1.shape());
  assert(array1.shape() == array2.shape());
  details::MaximumImplementation(array1, array2, ret);
}
template <typename T, typename C>
NDArray<T, C> Maximum(NDArray<T, C> const &array1, NDArray<T, C> const &array2)
{
  std::vector<std::size_t> return_shape(array1.shape());
  NDArray<T, C>            ret(return_shape);
  Maximum(array1, array2, ret);
  return ret;
}
template <typename T, typename C>
void Maximum(ShapelessArray<T, C> const &array1, ShapelessArray<T, C> const &array2,
             ShapelessArray<T, C> &ret)
{
  details::MaximumImplementation(array1, array2, ret);
}
template <typename T, typename C>
ShapelessArray<T, C> Maximum(ShapelessArray<T, C> const &array1, ShapelessArray<T, C> const &array2)
{
  ShapelessArray<T, C> ret(array1.size());
  Maximum(array1, array2, ret);
  return ret;
}

/**
 * return the product of all elements in the array
 * @tparam T
 * @tparam C
 * @param obj1
 * @param ret
 */
template <typename T, typename C>
void Product(ShapelessArray<T, C> const &obj1, T &ret)
{
  ret = obj1.data().in_parallel().Reduce(
      memory::TrivialRange(0, obj1.size()),
      [](typename ShapelessArray<T, C>::vector_register_type const &a,
         typename ShapelessArray<T, C>::vector_register_type const &b) ->
      typename ShapelessArray<T, C>::vector_register_type { return a * b; });
}
template <typename T, typename C>
T Product(ShapelessArray<T, C> const &obj1)
{
  T ret;
  Product(obj1, ret);
  return ret;
}
/**
 * return the product of all elements in the vector
 * @tparam T
 * @param obj1
 * @param ret
 */
template <typename T>
void Product(std::vector<T> const &obj1, T &ret)
{
  ret = std::accumulate(std::begin(obj1), std::end(obj1), std::size_t(1), std::multiplies<>());
}
template <typename T>
T Product(std::vector<T> const &obj1)
{
  T ret;
  Product(obj1, ret);
  return ret;
}

/**
 * return the product of all elements in the array
 * @tparam T
 * @tparam C
 * @param obj1
 * @param ret
 */
template <typename T, typename C>
void Sum(ShapelessArray<T, C> const &obj1, T &ret)
{
  ret = obj1.data().in_parallel().Reduce(
      memory::TrivialRange(0, obj1.size()),
      [](typename ShapelessArray<T, C>::vector_register_type const &a,
         typename ShapelessArray<T, C>::vector_register_type const &b) ->
      typename ShapelessArray<T, C>::vector_register_type { return a + b; });
}
template <typename T, typename C>
T Sum(ShapelessArray<T, C> const &obj1)
{
  T ret;
  Sum(obj1, ret);
  return ret;
}
template <typename T>
T Sum(Tensor<T> const &obj1)
{
  T ret(0);
  for (std::size_t j = 0; j < obj1.size(); ++j)
  {
    ret += obj1.At(j);
  }
  return ret;
}

/**
 * return the mean of all elements in the array
 * @tparam T
 * @tparam C
 * @param obj1
 * @param ret
 */
template <typename T, typename C>
void Mean(ShapelessArray<T, C> const &obj1, T &ret)
{

  Sum(obj1, ret);
  Divide(ret, T(obj1.size()), ret);
}
template <typename T, typename C>
T Mean(ShapelessArray<T, C> const &obj1)
{
  T ret;
  Mean(obj1, ret);
  return ret;
}

template <typename ArrayType>
void ReduceSum(ArrayType const &obj1, std::size_t axis, ArrayType &ret)
{
  assert((axis == 0) || (axis == 1));
  assert(obj1.shape().size() == 2);

  std::vector<std::size_t> access_idx{0, 0};
  if (axis == 0)
  {
    assert(ret.shape()[0] == 1);
    assert(ret.shape()[1] == obj1.shape()[1]);

    for (std::size_t i = 0; i < ret.size(); ++i)
    {
      ret[i] = typename ArrayType::Type(0);
      for (std::size_t j = 0; j < obj1.shape()[0]; ++j)
      {
        ret[i] += obj1({j, i});
      }
    }
  }
  else
  {
    assert(ret.shape()[0] == obj1.shape()[0]);
    assert(ret.shape()[1] == 1);

    for (std::size_t i = 0; i < ret.size(); ++i)
    {
      ret[i] = typename ArrayType::Type(0);
      for (std::size_t j = 0; j < obj1.shape()[1]; ++j)
      {
        ret[i] += obj1({i, j});
      }
    }
  }
}
template <typename ArrayType>
ArrayType ReduceSum(ArrayType const &obj1, std::size_t axis)
{
  assert((axis == 0) || (axis == 1));
  if (axis == 0)
  {
    std::vector<std::size_t> new_shape{1, obj1.shape()[1]};
    ArrayType                ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
  else
  {
    std::vector<std::size_t> new_shape{obj1.shape()[0], 1};
    ArrayType                ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
}

template <typename ArrayType>
ArrayType ReduceMean(ArrayType const &obj1, std::size_t const &axis)
{
  assert(axis == 0 || axis == 1);
  typename ArrayType::DataType n;
  if (axis == 0)
  {
    n = obj1.shape()[1];
  }
  else
  {
    n = obj1.shape()[0];
  }
  return Divide(ReduceSum(obj1, axis), n);
}

/**
 * Distance between max and min values in an array
 */
template <typename ArrayType>
void PeakToPeak(ArrayType arr)
{
  return Max(arr) - Min(arr);
}

/**
 * Finds the argument of the maximum value in an array
 * @tparam T data type
 * @tparam C container type
 * @param array array to search for maximum value
 * @return
 */
template <typename T, typename C>
void ArgMax(ShapelessArray<T, C> const &array, T &ret)
{
  ret          = 0;
  T cur_maxval = std::numeric_limits<T>::lowest();

  // just using ret as a free variable to store the current maxval for the loop here
  for (std::size_t i = 0; i < array.size(); ++i)
  {
    if (cur_maxval < array[i])
    {
      ret = static_cast<T>(i);
    }
  }
}
template <typename T, typename C>
T ArgMax(ShapelessArray<T, C> const &array)
{
  T ret;
  return ArgMax(array, ret);
}

template <typename T, typename C>
void Transpose(NDArray<T, C> &input_array, std::vector<std::size_t> const &perm)
{
  assert(perm.size() == input_array.shape().size());

  // set up an initial array
  NDArray<T, C> ret = input_array.Copy();

  NDArrayIterator<T, typename NDArray<T, C>::container_type> it_input(input_array);
  NDArrayIterator<T, typename NDArray<T, C>::container_type> it_ret(ret);

  it_ret.Transpose(perm);
  while (it_ret)
  {
    *it_input = *it_ret;
    ++it_input;
    ++it_ret;
  }

  std::vector<std::size_t> new_shape;
  for (std::size_t i = 0; i < perm.size(); ++i)
  {
    new_shape.push_back(input_array.shape()[perm[i]]);
  }
  input_array.Reshape(new_shape);
}

template <typename T, typename C>
void Transpose(NDArray<T, C> & /*input_array*/, NDArray<T, C> const & /*perm*/)
{
  // assert(perm.size() == input_array.shape().size());
}

template <typename ArrayType>
void Dot(ArrayType const &A, ArrayType const &B, ArrayType &ret)
{
  assert(A.shape().size() == 2);
  assert(B.shape().size() == 2);
  assert(A.shape()[1] == B.shape()[0]);

  for (std::size_t i(0); i < A.shape()[0]; ++i)
  {
    for (std::size_t j(0); j < B.shape()[1]; ++j)
    {
      ret.Get(std::vector<std::size_t>({i, j})) =
          A.Get(std::vector<std::size_t>({i, 0})) * B.Get(std::vector<std::size_t>({0, j}));
      for (std::size_t k(1); k < A.shape()[1]; ++k)
      {
        ret.Get(std::vector<std::size_t>({i, j})) +=
            A.Get(std::vector<std::size_t>({i, k})) * B.Get(std::vector<std::size_t>({k, j}));
      }
    }
  }
}

template <typename ArrayType>
ArrayType Dot(ArrayType const &A, ArrayType const &B)
{
  std::vector<std::size_t> return_shape{A.shape()[0], B.shape()[1]};
  ArrayType                ret(return_shape);
  Dot(A, B, ret);
  return ret;
}

/**
 * Naive routine for C = A.T(B)
 * @param A
 * @param B
 * @param ret
 * @return
 */
template <class ArrayType>
void DotTranspose(ArrayType const &A, ArrayType const &B, ArrayType &ret)
{
  assert(A.shape()[1] == B.shape()[1]);
  assert(A.shape()[0] == ret.shape()[0]);
  assert(B.shape()[0] == ret.shape()[1]);

  for (size_t i(0); i < A.shape()[0]; ++i)
  {
    for (size_t j(0); j < B.shape()[0]; ++j)
    {
      for (size_t k(0); k < A.shape()[1]; ++k)
      {
        ret.Get(std::vector<size_t>({i, j})) +=
            A.Get(std::vector<size_t>({i, k})) * B.Get(std::vector<size_t>({j, k}));
      }
    }
  }
}

template <typename ArrayType>
fetch::math::meta::IfIsMathShapeArray<ArrayType, ArrayType> DotTranspose(ArrayType const &A,
                                                                         ArrayType const &B)
{
  std::vector<std::size_t> return_shape{A.shape()[0], B.shape()[0]};
  ArrayType                ret(return_shape);
  DotTranspose(A, B, ret);
  return ret;
}

/**
 * Naive routine for C = T(A).B
 * @param A
 * @param B
 * @param ret
 * @return
 */
template <class ArrayType>
void TransposeDot(ArrayType const &A, ArrayType const &B, ArrayType &ret)
{
  assert(A.shape()[0] == B.shape()[0]);
  assert(A.shape()[1] == ret.shape()[0]);
  assert(B.shape()[1] == ret.shape()[1]);

  for (size_t i(0); i < A.shape()[1]; ++i)
  {
    for (size_t j(0); j < B.shape()[1]; ++j)
    {
      for (size_t k(0); k < A.shape()[0]; ++k)
      {
        ret.Get(std::vector<size_t>({i, j})) +=
            A.Get(std::vector<size_t>({k, i})) * B.Get(std::vector<size_t>({k, j}));
      }
    }
  }
}

template <class ArrayType>
ArrayType TransposeDot(ArrayType const &A, ArrayType const &B)
{
  std::vector<std::size_t> return_shape{A.shape()[1], B.shape()[1]};
  ArrayType                ret(return_shape);
  TransposeDot(A, B, ret);
  return ret;
}

/**
 * Adds a new dimension at a specified axis
 * @tparam T
 * @tparam C
 * @param input_array
 * @param axis
 */
template <typename T, typename C>
void ExpandDimensions(NDArray<T, C> &input_array, std::size_t const &axis)
{
  assert(axis <= input_array.shape().size());

  std::vector<std::size_t> new_shape;
  for (std::size_t i = 0; i <= input_array.shape().size(); ++i)
  {
    if (i < axis)
    {
      new_shape.push_back(input_array.shape()[i]);
    }
    else if (i == axis)
    {
      new_shape.push_back(1);
    }
    else
    {
      new_shape.push_back(input_array.shape()[i - 1]);
    }
  }

  input_array.Reshape(new_shape);
}
/**
 * The special case of axis = -1 is permissible, so we declare this function signature to capture it
 * @tparam T
 * @tparam C
 * @param input_array
 * @param axis
 */
template <typename T, typename C>
void ExpandDimensions(NDArray<T, C> &input_array, int const &axis)
{
  assert(axis <= static_cast<int>(input_array.size()));
  std::size_t new_axis;
  if (axis < 0)
  {
    assert(axis == -1);
    new_axis = input_array.shape().size();
  }
  else
  {
    new_axis = static_cast<std::size_t>(axis);
  }
  ExpandDimensions(input_array, new_axis);
}

/**
 * method for concatenating arrays
 */
namespace details {
template <typename ArrayType>
void ConcatImplementation(std::vector<ArrayType> const &input_arrays, ArrayType &ret)
{
  assert(input_arrays.size() > 0);

  std::size_t new_size = 0;
  for (std::size_t i = 0; i < input_arrays.size(); ++i)
  {
    new_size += input_arrays[i].size();
  }
  ret.Resize(new_size);

  if (input_arrays.size() == 1)
  {
    ret.Copy(input_arrays[0]);
  }
  else
  {
    std::size_t count = 0;
    for (std::size_t j = 0; j < input_arrays.size(); ++j)
    {
      for (std::size_t i = 0; i < input_arrays[j].size(); ++i, ++count)
      {
        ret[count] = input_arrays[j][i];
      }
    }
  }
}
}  // namespace details
template <typename T, typename C>
void Concat(ShapelessArray<T, C> &ret, std::vector<ShapelessArray<T, C>> const &input_arrays)
{
  details::ConcatImplementation(input_arrays, ret);
}
template <typename T, typename C>
ShapelessArray<T, C> Concat(std::vector<ShapelessArray<T, C>> const &input_arrays)
{
  ShapelessArray<T, C> ret;
  Concat(ret, input_arrays);
  return ret;
}

template <typename T, typename C>
void Concat(NDArray<T, C> &ret, std::vector<NDArray<T, C>> input_arrays, std::size_t const &axis)
{
  assert(input_arrays.size() > 0);
  assert(input_arrays[0].shape().size() > 0);

  if (input_arrays.size() == 1)
  {
    ret.ResizeFromShape(input_arrays[0].shape());
    ret.Copy(input_arrays[0]);
  }
  else
  {
    // figure out the size of the axis dim after concatenation
    std::size_t new_axis_dim = input_arrays[0].shape()[axis];
    assert(axis < input_arrays[0].shape().size());
    for (std::size_t i = 0; i < (input_arrays.size() - 1); ++i)
    {
      assert(input_arrays[i].shape() == input_arrays[i + 1].shape());
      new_axis_dim += input_arrays[i + 1].shape()[axis];
    }

    // figure out the size and shape of the output array
    std::vector<std::size_t> new_shape = {input_arrays[0].shape()};
    new_shape[axis]                    = new_axis_dim;
    ret.ResizeFromShape(new_shape);

    // identify the axis based stride
    std::size_t stride = input_arrays[0].shape()[axis];

    for (std::size_t j = 0; j < input_arrays.size(); ++j)
    {
      // figure out the part of the return array to fill with this input array
      std::vector<std::vector<std::size_t>> step{};
      for (std::size_t i = 0; i < ret.shape().size(); ++i)
      {
        if (i == axis)
        {
          step.push_back({j * stride, (j + 1) * stride, 1});
        }
        else
        {
          step.push_back({0, ret.shape()[i], 1});
        }
      }

      // copy the data across
      NDArrayIterator<T, C> ret_iterator{ret, step};
      NDArrayIterator<T, C> arr_iterator{input_arrays[j]};
      for (std::size_t k = 0; k < input_arrays[j].size(); ++k)
      {
        *ret_iterator = *arr_iterator;
        ++ret_iterator;
        ++arr_iterator;
      }
    }
  }
}
template <typename T, typename C>
NDArray<T, C> Concat(std::vector<NDArray<T, C>> input_arrays, std::size_t const & /*axis*/)
{
  NDArray<T, C> ret;
  Concat(ret, input_arrays);
  return ret;
}

/**
 * interleave data from multiple sources
 * @param x
 */
namespace details {
template <typename ArrayType>
void DynamicStitchImplementation(ArrayType &input_array, ArrayType const &indices,
                                 ArrayType const &data)
{
  input_array.LazyResize(indices.size());

  // loop through all output data locations identifying the next data point to copy into it
  for (std::size_t i = 0; i < indices.size(); ++i)  // iterate through lists of indices
  {
    input_array.Set(std::size_t(indices[i]), data[i]);
  }
}
}  // namespace details
template <typename T, typename C>
void DynamicStitch(ShapelessArray<T, C> &input_array, ShapelessArray<T, C> const &indices,
                   ShapelessArray<T, C> const &data)
{
  details::DynamicStitchImplementation(input_array, indices, data);
}
template <typename T, typename C>
void DynamicStitch(NDArray<T, C> &input_array, NDArray<T, C> &indices, NDArray<T, C> &data)
{
  //  input_array.MajorOrderFlip();
  indices.MajorOrderFlip();
  data.MajorOrderFlip();

  details::DynamicStitchImplementation(input_array, indices, data);

  input_array.MajorOrderFlip();
}

}  // namespace math
}  // namespace fetch
