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

#include "math/comparison.hpp"
#include "math/fundamental_operators.hpp"  // add, subtract etc.
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {

template <typename T>
class Tensor;

template <typename T, typename SizeType>
class TensorIterator;

/**
 * applies bit mask to one array storing result in new array
 * @param x
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> BooleanMask(ArrayType &input_array, ArrayType const &mask,
                                                 ArrayType &ret)
{
  ASSERT(input_array.size() == mask.size());
  ASSERT(ret.size() == Sum(mask));

  typename ArrayType::SizeType counter = 0;
  for (typename ArrayType::SizeType i = 0; i < input_array.size(); ++i)
  {
    // TODO(private issue 193): implement boolean only array
    ASSERT((mask.At(i) == 1) || (mask.At(i) == 0));
    if (bool(mask.At(i)))
    {
      ret.At(counter) = input_array.At(i);
      ++counter;
    }
  }
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
  for (T &e : array)
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
template <typename T>
void Max(Tensor<T> const &array, typename Tensor<T>::SizeType const &axis, Tensor<T> &ret)
{
  ASSERT(array.shape().size() <= 2);
  ASSERT(axis < array.shape().size());

  if (array.shape().size() == 1)
  {
    ASSERT(axis == 0);

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
 * Min function returns the smalled value in the array
 * @tparam ArrayType input array type
 * @tparam T input scalar return param
 * @param array input array
 * @param ret return value
 */
template <typename ArrayType, typename T>
meta::IfIsValidArrayScalarPair<ArrayType, T, void> Min(ArrayType const &array, T &ret)
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
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, typename ArrayType::Type> Min(ArrayType const &array)
{
  typename ArrayType::Type ret;
  Min(array, ret);
  return ret;
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

  typename ArrayType::SizeType idx{0};
  for (auto &ret_val : ret)
  {
    ret_val = Max(array1.at(idx), array2.at(idx));
    ++idx;
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
meta::IfIsMathArray<ArrayType, void> ReduceSum(ArrayType const &            obj1,
                                               typename ArrayType::SizeType axis, ArrayType &ret)
{
  ASSERT((axis == 0) || (axis == 1));
  ASSERT(obj1.shape().size() == 2);

  std::vector<typename ArrayType::SizeType> access_idx{0, 0};
  if (axis == 0)
  {
    ASSERT(ret.shape()[0] == 1);
    ASSERT(ret.shape()[1] == obj1.shape()[1]);

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
    ASSERT(ret.shape()[0] == obj1.shape()[0]);
    ASSERT(ret.shape()[1] == 1);

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
meta::IfIsMathArray<ArrayType, ArrayType> ReduceSum(ArrayType const &            obj1,
                                                    typename ArrayType::SizeType axis)
{
  ASSERT((axis == 0) || (axis == 1));
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
void ArgMax(ArrayType const &array, ArrayType &ret, typename ArrayType::SizeType axis = 0)
{
  ASSERT((array.shape().size() == 1) || (array.shape().size() == 2));
  ASSERT((axis == 0) || (axis == 1));

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
  ASSERT((array.shape().size() == 1) || (array.shape().size() == 2));
  ASSERT((axis == 0) || (axis == 1));

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
fetch::math::meta::IfIsMathArray<ArrayType, void> Dot(ArrayType const &A, ArrayType const &B,
                                                      ArrayType &ret)
{
  ASSERT(A.shape().size() == 2);
  ASSERT(B.shape().size() == 2);
  ASSERT(A.shape()[1] == B.shape()[0]);

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
        ret.At(std::vector<typename ArrayType::SizeType>({i, j})) +=
            A.At(std::vector<typename ArrayType::SizeType>({i, k})) *
            B.At(std::vector<typename ArrayType::SizeType>({j, k}));
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
        ret.At(std::vector<typename ArrayType::SizeType>({i, j})) +=
            A.At(std::vector<typename ArrayType::SizeType>({k, i})) *
            B.At(std::vector<typename ArrayType::SizeType>({k, j}));
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
  for (typename ArrayType::SizeType i = 0; i < indices.size();
       ++i)  // iterate through lists of indices
  {
    input_array.Set(typename ArrayType::SizeType(indices.At(i)), data.At(i));
  }
}

}  // namespace math
}  // namespace fetch
