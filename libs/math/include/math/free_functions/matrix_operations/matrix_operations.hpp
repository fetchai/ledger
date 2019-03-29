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

template <typename T>
class Tensor;

template <typename T, typename SizeType>
class TensorIterator;

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

  typename ArrayType::SizeType counter = 0;
  for (typename ArrayType::SizeType i = 0; i < input_array.size(); ++i)
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

/*
 * Adds sparse updates to the variable referenced
 */
namespace details {
template <typename ArrayType>
void ScatterImplementation(ArrayType &input_array, ArrayType &updates, ArrayType &indices)
{
  // sort indices and updates into ascending order
  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> AB;

  // copy into pairs
  // Note that A values are put in "first" this is very important
  for (typename ArrayType::SizeType i = 0; i < updates.size(); ++i)
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
  typename ArrayType::SizeType arr_count = 0;
  for (typename ArrayType::SizeType count = 0; count < indices.size(); ++count)
  {
    // TODO(private issue 282): Think about this code
    while (arr_count < static_cast<typename ArrayType::SizeType>(indices[count]))
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

template <typename ArrayType, typename T>
meta::IfIsMathArray<ArrayType, void> Max(ArrayType const &array, T &ret)
{
  ret = std::numeric_limits<T>::lowest();
  for (T &e : array)
  {
    if (e > ret)
    {
      ret = e;
    }
  }
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
        std::numeric_limits<typename ShapelessArray<T, C>::Type>::lowest();
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
template <typename T>
void Max(Tensor<T> const &array, typename Tensor<T>::SizeType const &axis, Tensor<T> &ret)
{
  assert(array.shape().size() <= 2);
  assert(axis < array.shape().size());

  if (array.shape().size() == 1)
  {
    assert(axis == 0);

    T cur_max = std::numeric_limits<T>::lowest();
    for (T &e : array)
    {
      if (e > cur_max)
      {
        cur_max = e;
      }
    }
    ret[0] = cur_max;
  }
  else
  {
    typename Tensor<T>::SizeType off_axis = 0;
    if (axis == 0)
    {
      off_axis = 1;
    }
    else
    {
      off_axis = 0;
    }

    for (std::size_t j = 0; j < array.shape()[off_axis]; ++j)
    {
      T cur_max = std::numeric_limits<T>::lowest();
      for (T &e : array.Slice(j))
      {
        if (e > cur_max)
        {
          cur_max = e;
        }
      }
      ret[j] = cur_max;
    }
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

template <typename ArrayType, typename T>
meta::IfIsMathArray<ArrayType, void> Min(ArrayType const &array, T &ret)
{
  ret = std::numeric_limits<T>::max();
  for (T &e : array)
  {
    if (ret < e)
    {
      ret = e;
    }
  }
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

  for (typename ArrayType::SizeType i = 0; i < ret.size(); ++i)
  {
    ret[i] = std::max(array1[i], array2[i]);
  }
  return ret;
}
}  // namespace details

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
  Sum(obj1, ret);
  return ret;
}
template <typename T>
void Sum(Tensor<T> const &obj1, T &ret)
{
  for (typename Tensor<T>::SizeType j = 0; j < obj1.size(); ++j)
  {
    ret += obj1.At(j);
  }
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
void ReduceSum(ArrayType const &obj1, typename ArrayType::SizeType axis, ArrayType &ret)
{
  assert((axis == 0) || (axis == 1));
  assert(obj1.shape().size() == 2);

  std::vector<typename ArrayType::SizeType> access_idx{0, 0};
  if (axis == 0)
  {
    assert(ret.shape()[0] == 1);
    assert(ret.shape()[1] == obj1.shape()[1]);

    for (typename ArrayType::SizeType i = 0; i < ret.size(); ++i)
    {
      ret[i] = typename ArrayType::Type(0);
      for (typename ArrayType::SizeType j = 0; j < obj1.shape()[0]; ++j)
      {
        ret[i] += obj1({j, i});
      }
    }
  }
  else
  {
    assert(ret.shape()[0] == obj1.shape()[0]);
    assert(ret.shape()[1] == 1);

    for (typename ArrayType::SizeType i = 0; i < ret.size(); ++i)
    {
      ret[i] = typename ArrayType::Type(0);
      for (typename ArrayType::SizeType j = 0; j < obj1.shape()[1]; ++j)
      {
        ret[i] += obj1({i, j});
      }
    }
  }
}
template <typename ArrayType>
ArrayType ReduceSum(ArrayType const &obj1, typename ArrayType::SizeType axis)
{
  assert((axis == 0) || (axis == 1));
  if (axis == 0)
  {
    std::vector<typename ArrayType::SizeType> new_shape{1, obj1.shape()[1]};
    ArrayType                                 ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
  else
  {
    std::vector<typename ArrayType::SizeType> new_shape{obj1.shape()[0], 1};
    ArrayType                                 ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
}

template <typename ArrayType>
ArrayType ReduceMean(ArrayType const &obj1, typename ArrayType::SizeType const &axis)
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
template <typename ArrayType>
void ArgMax(ArrayType const &array, ArrayType &ret, typename ArrayType::SizeType axis = 0)
{
  assert((array.shape().size() == 1) || (array.shape().size() == 2));
  assert((axis == 0) || (axis == 1));

  typename ArrayType::Type cur_maxval = std::numeric_limits<typename ArrayType::Type>::lowest();

  if (array.shape().size() == 1)
  {
    // just using ret as a free variable to store the current maxval for the loop here
    for (typename ArrayType::SizeType i(0); i < array.size(); ++i)
    {
      if (cur_maxval < array[i])
      {
        cur_maxval = array.At(i);
        ret.At(0)  = typename ArrayType::Type(i);
      }
    }
  }
  else
  {
    if (axis == 0)
    {
      // get arg max for each row indexing by axis 0
      for (std::size_t j = 0; j < array.shape().at(axis); ++j)
      {
        ret.At(j) = ArgMax(array.Slice(j), axis).At(0);
      }
    }
    else
    {
      throw std::runtime_error(
          "Argmax for axis == 1 not yet implemented; depends upon arbitrary dimension slicing for "
          "tensor");
    }
  }
}
template <typename ArrayType>
ArrayType ArgMax(ArrayType const &array, typename ArrayType::SizeType axis = 0)
{
  assert((array.shape().size() == 1) || (array.shape().size() == 2));
  assert((axis == 0) || (axis == 1));

  ArrayType ret;
  if (array.shape().size() == 1)
  {
    // 1D argmax result has size 1
    ret = ArrayType{1};
  }
  else
  {
    // 2D argmax result has size
    if (axis == 0)
    {
      ret = ArrayType{{array.shape().at(axis), typename ArrayType::SizeType(1)}};
    }
    else
    {
      ret = ArrayType{{typename ArrayType::SizeType(1), array.shape().at(axis)}};
    }
  }

  ArgMax(array, ret, axis);
  return ret;
}

template <typename ArrayType>
void Dot(ArrayType const &A, ArrayType const &B, ArrayType &ret)
{
  assert(A.shape().size() == 2);
  assert(B.shape().size() == 2);
  assert(A.shape()[1] == B.shape()[0]);

  for (typename ArrayType::SizeType i(0); i < A.shape()[0]; ++i)
  {
    for (typename ArrayType::SizeType j(0); j < B.shape()[1]; ++j)
    {
      ret.At(std::vector<typename ArrayType::SizeType>({i, j})) =
          A.At(std::vector<typename ArrayType::SizeType>({i, 0})) *
          B.At(std::vector<typename ArrayType::SizeType>({0, j}));
      for (typename ArrayType::SizeType k(1); k < A.shape()[1]; ++k)
      {
        ret.At(std::vector<typename ArrayType::SizeType>({i, j})) +=
            A.At(std::vector<typename ArrayType::SizeType>({i, k})) *
            B.At(std::vector<typename ArrayType::SizeType>({k, j}));
      }
    }
  }
}

template <typename ArrayType>
ArrayType Dot(ArrayType const &A, ArrayType const &B)
{
  std::vector<typename ArrayType::SizeType> return_shape{A.shape()[0], B.shape()[1]};
  ArrayType                                 ret(return_shape);
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
  assert(A.shape().size() == 2);
  assert(B.shape().size() == 2);
  assert(A.shape()[1] == B.shape()[1]);
  assert(A.shape()[0] == ret.shape()[0]);
  assert(B.shape()[0] == ret.shape()[1]);

  for (size_t i(0); i < A.shape()[0]; ++i)
  {
    for (size_t j(0); j < B.shape()[0]; ++j)
    {
      for (size_t k(0); k < A.shape()[1]; ++k)
      {
        ret.At(std::vector<typename ArrayType::SizeType>({i, j})) +=
            A.At(std::vector<typename ArrayType::SizeType>({i, k})) *
            B.At(std::vector<typename ArrayType::SizeType>({j, k}));
      }
    }
  }
}

template <typename ArrayType>
fetch::math::meta::IfIsMathShapeArray<ArrayType, ArrayType> DotTranspose(ArrayType const &A,
                                                                         ArrayType const &B)
{
  std::vector<typename ArrayType::SizeType> return_shape{A.shape()[0], B.shape()[0]};
  ArrayType                                 ret(return_shape);
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
        ret.At(std::vector<typename ArrayType::SizeType>({i, j})) +=
            A.At(std::vector<typename ArrayType::SizeType>({k, i})) *
            B.At(std::vector<typename ArrayType::SizeType>({k, j}));
      }
    }
  }
}

template <class ArrayType>
ArrayType TransposeDot(ArrayType const &A, ArrayType const &B)
{
  std::vector<typename ArrayType::SizeType> return_shape{A.shape()[1], B.shape()[1]};
  ArrayType                                 ret(return_shape);
  TransposeDot(A, B, ret);
  return ret;
}

/**
 * method for concatenating arrays
 */
namespace details {
template <typename ArrayType>
void ConcatImplementation(std::vector<ArrayType> const &input_arrays, ArrayType &ret)
{
  assert(input_arrays.size() > 0);

  typename ArrayType::SizeType new_size = 0;
  for (typename ArrayType::SizeType i = 0; i < input_arrays.size(); ++i)
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
    typename ArrayType::SizeType count = 0;
    for (typename ArrayType::SizeType j = 0; j < input_arrays.size(); ++j)
    {
      for (typename ArrayType::SizeType i = 0; i < input_arrays[j].size(); ++i, ++count)
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
  for (typename ArrayType::SizeType i = 0; i < indices.size();
       ++i)  // iterate through lists of indices
  {
    input_array.Set(typename ArrayType::SizeType(indices[i]), data[i]);
  }
}
}  // namespace details
template <typename T, typename C>
void DynamicStitch(ShapelessArray<T, C> &input_array, ShapelessArray<T, C> const &indices,
                   ShapelessArray<T, C> const &data)
{
  details::DynamicStitchImplementation(input_array, indices, data);
}

}  // namespace math
}  // namespace fetch
