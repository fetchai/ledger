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
#include <numeric>

#include "math/base_types.hpp"
#include "math/comparison.hpp"
#include "math/fundamental_operators.hpp"  // add, subtract etc.
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {
//
// template <typename T, typename C>
// class Tensor;
//
// template <typename T, typename C, typename TensorType>
// class TensorIterator;

// TODO - vectorisation implementations not yet called
namespace details_vectorisation {

/**
 * Min function for returning the smallest value in an array
 * @tparam ArrayType
 * @param array
 * @return
 */
template <typename ArrayType>
inline void Min(ArrayType const &array, typename ArrayType::Type &ret)
{
  using vector_register_type = typename ArrayType::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });
}

/**
 * return the product of all elements in the array
 * @tparam T
 * @tparam C
 * @param obj1
 * @param ret
 */
template <typename ArrayType>
void Product(ArrayType const &obj1, typename ArrayType::Type &ret)
{
  ret =
      obj1.data().in_parallel().Reduce(memory::TrivialRange(0, obj1.size()),
                                       [](typename ArrayType::vector_register_type const &a,
                                          typename ArrayType::vector_register_type const &b) ->
                                       typename ArrayType::vector_register_type { return a * b; });
}

/**
 * return the product of all elements in the array
 * @tparam T
 * @tparam C
 * @param obj1
 * @param ret
 */
template <typename ArrayType>
void Sum(ArrayType const &obj1, typename ArrayType::Type &ret)
{
  ret =
      obj1.data().in_parallel().Reduce(memory::TrivialRange(0, obj1.size()),
                                       [](typename ArrayType::vector_register_type const &a,
                                          typename ArrayType::vector_register_type const &b) ->
                                       typename ArrayType::vector_register_type { return a + b; });
}

}  // namespace details_vectorisation

/**
 * applies bit mask to one array storing result in new array
 * @param x
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> BooleanMask(ArrayType const &input_array,
                                                 ArrayType const &mask, ArrayType &ret)
{
  ASSERT(input_array.size() == mask.size());
  ASSERT(ret.size() == Sum(mask));

  auto     it1 = input_array.cbegin();
  auto     it2 = mask.cbegin();
  auto     rit = ret.begin();
  SizeType counter{0};
  while (it1.is_valid())
  {
    // TODO(private issue 193): implement boolean only array
    ASSERT((*it2 == 1) || (*it2 == 0));
    if (*it2)
    {
      *rit = *it1;
      ++counter;
    }
    ++it1;
    ++it2;
    ++rit;
  }

  ret.LazyResize(counter);
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> BooleanMask(ArrayType &input_array, ArrayType const &mask)
{
  ArrayType ret{Sum(mask)};
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
template <typename ArrayType>
void Scatter(ArrayType &input_array, ArrayType const &updates, ArrayType const &indices)
{
  ASSERT(updates.size() == indices.size());
  ASSERT(input_array.size() >= Max(indices));

  typename ArrayType::SizeType idx{0};
  for (auto &update_val : updates)
  {
    input_array.At(indices.At(idx)) = update_val;
  }
}

/**
 * Finds the maximum value in an array
 * @tparam ArrayType
 * @tparam T
 * @param array
 * @param ret
 * @return
 */
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Max(ArrayType const &array, T &ret)
{
  ret = std::numeric_limits<T>::lowest();
  for (T const &e : array)
  {
    if (e > ret)
    {
      ret = e;
    }
  }
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, typename ArrayType::Type> Max(ArrayType const &array)
{
  typename ArrayType::Type ret;
  Max(array, ret);
  return ret;
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
template <typename ArrayType>
void Max(ArrayType const &array, typename ArrayType::SizeType const &axis, ArrayType &ret)
{
  ASSERT(array.shape().size() <= 2);
  ASSERT(axis < array.shape().size());

  if (array.shape().size() == 1)
  {
    ASSERT(axis == 0);
    ret[0] = Max(array);
  }
  else
  {  // Argmax along a single axis
    SizeType axis_length = array.shape()[axis];
    ASSERT(axis_length > 1);
    ASSERT(ret.size() == Divide(Product(array.shape()), array.shape()[axis]));

    // fill the return with the first index values
    ret.Assign(array.Slice(0, axis));

    //
    for (SizeType n{1}; n < axis_length; ++n)
    {
      auto cur_slice    = array.Slice(n, axis);
      auto cur_slice_it = cur_slice.begin();
      auto rit          = ret.begin();

      // check every element in the n-1 dimensional return
      while (cur_slice_it.is_valid())
      {
        if (*cur_slice_it > *rit)
        {
          *rit = *cur_slice_it;
        }
        ++rit;
        ++cur_slice_it;
      }
    }
  }
}
template <typename T>
void Max(std::vector<T> const &obj1, T &ret)
{
  ret = *std::max_element(std::begin(obj1), std::end(obj1));
}
template <typename T>
T Max(std::vector<T> const &obj1)
{
  T ret;
  Max(obj1, ret);
  return ret;
}

/**
 * Min function returns the smalled value in the array
 * @tparam ArrayType input array type
 * @tparam T input scalar return param
 * @param array input array
 * @param ret return value
 */
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Min(ArrayType const &array, T &ret)
{
  ret = std::numeric_limits<T>::max();
  for (T const &e : array)
  {
    if (ret < e)
    {
      ret = e;
    }
  }
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, typename ArrayType::Type> Min(ArrayType const &array)
{
  typename ArrayType::Type ret;
  Min(array, ret);
  return ret;
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
template <typename ArrayType>
void Min(ArrayType const &array, typename ArrayType::SizeType const &axis, ArrayType &ret)
{
  ASSERT(array.shape().size() <= 2);
  ASSERT(axis < array.shape().size());

  if (array.shape().size() == 1)
  {
    ASSERT(axis == 0);
    ret[0] = Min(array);
  }
  else
  {  // Argmax along a single axis
    SizeType axis_length = array.shape()[axis];
    ASSERT(axis_length > 1);
    ASSERT(ret.size() == Divide(Product(array.shape()), array.shape()[axis]));

    // fill the return with the first index values
    ret.Assign(array.Slice(0, axis));

    //
    for (SizeType n{1}; n < axis_length; ++n)
    {
      auto cur_slice    = array.Slice(n, axis);
      auto cur_slice_it = cur_slice.begin();
      auto rit          = ret.begin();

      // check every element in the n-1 dimensional return
      while (cur_slice_it.is_valid())
      {
        if (*cur_slice_it < *rit)
        {
          *rit = *cur_slice_it;
        }
        ++rit;
        ++cur_slice_it;
      }
    }
  }
}

/**
 * Returns an array containing the elementwise maximum of two other ndarrays
 * @param x array input 1
 * @param y array input 2
 * @return the combined array
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Maximum(ArrayType const &array1, ArrayType const &array2,
                                             ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(ret.shape() == array2.shape());

  auto it1 = array1.cbegin();
  auto it2 = array2.cbegin();
  auto rit = ret.begin();

  while (it1.is_valid())
  {
    *rit = (*it1 > *it2) ? *it1 : *it2;
    ++it1;
    ++it2;
    ++rit;
  }
  return ret;
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Maximum(ArrayType const &array1, ArrayType const &array2)
{
  ArrayType ret(array1.shape());
  Maximum(array1, array2, ret);
  return ret;
}

/**
 * returns the product of all values in one array
 * @tparam ArrayType
 * @tparam T
 * @param array1
 * @param ret
 * @return
 */
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Product(ArrayType const &array1, T &ret)
{
  ret = typename ArrayType::Type(1);
  for (auto &val : array1)
  {
    ret *= val;
  }
}

template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, T> Product(ArrayType const &array1)
{
  T ret;
  Product(array1, ret);
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
 * return the sum of all elements in the array
 * @tparam T
 * @tparam C
 * @param obj1
 * @param ret
 */
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Sum(ArrayType const &array1, T &ret)
{
  ret = typename ArrayType::Type(0);
  for (auto &val : array1)
  {
    ret += val;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, typename ArrayType::Type> Sum(ArrayType const &array1)
{
  typename ArrayType::Type ret;
  Sum(array1, ret);
  return ret;
}

template <typename ArrayType>
void ReduceSum(ArrayType const &obj1, SizeType axis, ArrayType &ret)
{
  ASSERT((axis == 0) || (axis == 1));
  ASSERT(obj1.shape().size() == 2);

  SizeVector access_idx{0, 0};
  if (axis == 0)
  {
    ASSERT(ret.shape()[0] == 1);
    ASSERT(ret.shape()[1] == obj1.shape()[1]);

    for (SizeType i = 0; i < ret.size(); ++i)
    {
      ret[i] = typename ArrayType::Type(0);
      for (SizeType j = 0; j < obj1.shape()[0]; ++j)
      {
        ret[i] += obj1(j, i);
      }
    }
  }
  else
  {
    ASSERT(ret.shape()[0] == obj1.shape()[0]);
    ASSERT(ret.shape()[1] == 1);

    for (SizeType i = 0; i < ret.size(); ++i)
    {
      ret[i] = typename ArrayType::Type(0);
      for (SizeType j = 0; j < obj1.shape()[1]; ++j)
      {
        ret[i] += obj1(i, j);
      }
    }
  }
}

template <typename ArrayType>
ArrayType ReduceSum(ArrayType const &obj1, SizeType axis)
{
  ASSERT((axis == 0) || (axis == 1));
  if (axis == 0)
  {
    SizeVector new_shape{1, obj1.shape()[1]};
    ArrayType  ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
  else
  {
    SizeVector new_shape{obj1.shape()[0], 1};
    ArrayType  ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> ReduceMean(ArrayType const &                   obj1,
                                                     typename ArrayType::SizeType const &axis)
{
  ASSERT(axis == 0 || axis == 1);
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
meta::IfIsMathArray<ArrayType, void> ArgMax(ArrayType const &array, ArrayType &ret,
                                            SizeType axis = NO_AXIS)
{
  using Type = typename ArrayType::Type;

  if (axis == NO_AXIS)
  {  // Argmax over the full array
    ASSERT(ret.size() == SizeType(1));
    SizeType position = 0;
    auto     it       = array.begin();
    Type     value    = std::numeric_limits<Type>::lowest();
    while (it.is_valid())
    {
      if (*it > value)
      {
        value    = *it;
        position = it.counter();
      }
      ++it;
    }

    ret[0] = static_cast<Type>(position);
  }
  else
  {  // Argmax along a single axis
    SizeType axis_length = array.shape()[axis];
    ASSERT(axis_length > 1);
    ASSERT(ret.size() == Divide(Product(array.shape()), array.shape()[axis]));

    ret.Fill(Type(0));
    auto max_slice = (array.Slice(0, axis)).Copy();

    for (SizeType n{1}; n < axis_length; ++n)
    {
      auto cur_slice = array.Slice(n, axis);

      auto max_slice_it = max_slice.begin();
      auto cur_slice_it = cur_slice.begin();
      auto ret_it       = ret.begin();

      // check every element in the n-1 dimensional return
      while (max_slice_it.is_valid())
      {
        if (*cur_slice_it > *max_slice_it)
        {
          *ret_it       = static_cast<Type>(n);
          *max_slice_it = *cur_slice_it;
        }
        ++ret_it;
        ++cur_slice_it;
        ++max_slice_it;
      }
    }
  }
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> ArgMax(ArrayType const &array, SizeType axis = 0)
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
    SizeVector ret_shape = array.shape();
    ret_shape.erase(ret_shape.begin() + int(axis));
    ret = ArrayType{ret_shape};
  }

  ArgMax(array, ret, axis);
  return ret;
}

template <typename T>
void ArgMax(std::vector<T> const &obj1, T &ret)
{
  ret = T(std::distance(std::begin(obj1), std::max_element(std::begin(obj1), std::end(obj1))));
}
template <typename T>
T ArgMax(std::vector<T> const &obj1)
{
  T ret;
  ArgMax(obj1, ret);
  return ret;
}

template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> Dot(ArrayType const &A, ArrayType const &B,
                                                      ArrayType &ret)
{
  ASSERT(A.shape().size() == 2);
  ASSERT(B.shape().size() == 2);
  ASSERT(A.shape()[1] == B.shape()[0]);

  for (SizeType i(0); i < A.shape()[0]; ++i)
  {
    for (SizeType j(0); j < B.shape()[1]; ++j)
    {
      ret.At(i, j) = A.At(i, 0) * B.At(0, j);
      for (SizeType k(1); k < A.shape()[1]; ++k)
      {
        ret.At(i, j) += A.At(i, k) * B.At(k, j);
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
fetch::math::meta::IfIsMathArray<ArrayType, void> DotTranspose(ArrayType const &A,
                                                               ArrayType const &B, ArrayType &ret)
{
  ASSERT(A.shape().size() == 2);
  ASSERT(B.shape().size() == 2);
  ASSERT(A.shape()[1] == B.shape()[1]);
  ASSERT(A.shape()[0] == ret.shape()[0]);
  ASSERT(B.shape()[0] == ret.shape()[1]);

  for (size_t i(0); i < A.shape()[0]; ++i)
  {
    for (size_t j(0); j < B.shape()[0]; ++j)
    {
      for (size_t k(0); k < A.shape()[1]; ++k)
      {
        ret.At(i, j) += A.At(i, k) * B.At(j, k);
      }
    }
  }
}

template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> DotTranspose(ArrayType const &A,
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
fetch::math::meta::IfIsMathArray<ArrayType, void> TransposeDot(ArrayType const &A,
                                                               ArrayType const &B, ArrayType &ret)
{
  ASSERT(A.shape()[0] == B.shape()[0]);
  ASSERT(A.shape()[1] == ret.shape()[0]);
  ASSERT(B.shape()[1] == ret.shape()[1]);

  for (size_t i(0); i < A.shape()[1]; ++i)
  {
    for (size_t j(0); j < B.shape()[1]; ++j)
    {
      for (size_t k(0); k < A.shape()[0]; ++k)
      {
        ret.At(i, j) += A.At(k, i) * B.At(k, j);
      }
    }
  }
}

template <class ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> TransposeDot(ArrayType const &A,
                                                                    ArrayType const &B)
{
  std::vector<typename ArrayType::SizeType> return_shape{A.shape()[1], B.shape()[1]};
  ArrayType                                 ret(return_shape);
  TransposeDot(A, B, ret);
  return ret;
}

template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> DynamicStitch(ArrayType &      input_array,
                                                                ArrayType const &indices,
                                                                ArrayType const &data)
{
  ASSERT(data.size() <= input_array.size());
  ASSERT(input_array.size() >= Max(indices));
  ASSERT(Min(indices) >= 0);
  input_array.LazyResize(indices.size());

  auto ind_it  = indices.cbegin();
  auto data_it = data.cbegin();

  while (data_it.is_valid())
  {
    // loop through all output data locations identifying the next data point to copy into it
    for (SizeType i = 0; i < indices.size(); ++i)  // iterate through lists of indices
    {
      input_array.Set(SizeType(*ind_it), *data_it);
    }
  }
}

template <typename ArrayType>
void Concat(ArrayType &ret, std::vector<ArrayType> const &input_arrays)
{
  ASSERT(input_arrays.size() > 0);

  SizeType new_size = 0;
  for (SizeType i = 0; i < input_arrays.size(); ++i)
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
    SizeType count = 0;
    for (SizeType j = 0; j < input_arrays.size(); ++j)
    {
      for (SizeType i = 0; i < input_arrays[j].size(); ++i, ++count)
      {
        ret[count] = input_arrays[j][i];
      }
    }
  }
}
template <typename ArrayType>
ArrayType Concat(std::vector<ArrayType> const &input_arrays)
{
  ArrayType ret;
  Concat(ret, input_arrays);
  return ret;
}

}  // namespace math
}  // namespace fetch
