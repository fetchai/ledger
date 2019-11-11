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
#include "math/exceptions/exceptions.hpp"
#include "math/fundamental_operators.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemm_nn_novector.hpp"
#include "math/linalg/blas/gemm_nn_vector.hpp"
#include "math/linalg/blas/gemm_nt_novector.hpp"
#include "math/linalg/blas/gemm_nt_vector.hpp"
#include "math/linalg/blas/gemm_tn_novector.hpp"
#include "math/linalg/blas/gemm_tn_vector.hpp"
#include "math/linalg/prototype.hpp"
#include "math/meta/math_type_traits.hpp"
#include "math/tensor_reduce.hpp"
#include "vectorise/math/standard_functions.hpp"

#include <cassert>
#include <numeric>
#include <vector>

namespace fetch {
namespace math {

// TODO (private 854) - vectorisation implementations not yet called
namespace details_vectorisation {

/**
 * Min function for returning the smallest value in an array
 * @tparam ArrayType
 * @param array
 * @return
 */
template <typename ArrayType>
void Min(ArrayType const &array, typename ArrayType::Type &ret)
{
  ret = array.data().in_parallel().Reduce(memory::Range(0, array.size()),
                                          [](auto const &a, auto const &b) { return min(a, b); });
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
  // TODO(private issue 994): Create test for this function
  if (obj1.padding() == 1)
  {
    ret = obj1.data().in_parallel().Reduce(memory::Range(0, obj1.size()),
                                           [](auto const &a, auto const &b) { return a * b; });
  }
  else
  {
    auto it1 = obj1.cbegin();
    ret      = static_cast<typename ArrayType::Type>(1);
    while (it1.is_valid())
    {
      ret *= (*it1);
      ++it1;
    }
  }
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
  ret = obj1.data().in_parallel().Reduce(memory::Range(0, obj1.size()),
                                         [](auto const &a, auto const &b) { return a + b; });
}

}  // namespace details_vectorisation

/**
 * applies elementwise switch mask to arrays
 * @tparam ArrayType
 * @param mask : the condition boolean array
 * @param then_array : if condition is true then
 * @param else_array : if condition is not true then
 * @param ret : return array
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Switch(ArrayType const &mask, ArrayType const &then_array,
                                            ArrayType const &else_array, ArrayType &ret)
{
  using DataType = typename ArrayType::Type;

  assert(else_array.size() == then_array.size());
  assert(ret.size() == then_array.size());

  ArrayType inverse_mask = mask.Copy();
  Subtract(static_cast<DataType>(1), mask, inverse_mask);

  ArrayType intermediate = ret.Copy();
  Multiply(then_array, mask, ret);
  Multiply(else_array, inverse_mask, intermediate);
  Add(ret, intermediate, ret);
}

/**
 * applies elementwise switch mask to arrays
 * @param x
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Switch(ArrayType const &mask, ArrayType const &then_array,
                                                 ArrayType const &else_array)
{
  ArrayType ret(mask.shape());
  Switch(mask, then_array, else_array, ret);
  return ret;
}

/**
 * applies bit mask to one array storing result in new array
 * @param x
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> BooleanMask(ArrayType const &input_array,
                                                 ArrayType const &mask, ArrayType &ret)
{
  // TODO (#1453) this is a wrong implementation of boolean mask, the return array will always be
  // all zero unless all conditions are true
  assert(input_array.size() == mask.size());
  assert(ret.size() >= typename ArrayType::SizeType(Sum(mask)));

  auto     it1 = input_array.cbegin();
  auto     it2 = mask.cbegin();
  auto     rit = ret.begin();
  SizeType counter{0};
  while (rit.is_valid())
  {
    // TODO(private issue 193): implement boolean only array
    assert((*it2 == 1) || (*it2 == 0));
    if (static_cast<uint64_t>(*it2))
    {
      *rit = *it1;
      ++counter;
    }
    ++it1;
    ++it2;
    ++rit;
  }

  ret.Resize({counter});
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> BooleanMask(ArrayType &input_array, ArrayType const &mask)
{
  ArrayType ret{typename ArrayType::SizeType(Sum(mask))};
  BooleanMask(input_array, mask, ret);
  return ret;
}

/**
 * Scatter updates data in the input array at locations specified by indices
 * with values specified by updates
 * @tparam ArrayType Tensor of some data type
 * @tparam Indices indices which are to be updated
 * @param input_array the input tensor to update
 * @param updates the update values to apply
 * @param indices vector of indices at which to apply the updates
 */
template <typename ArrayType>
void Scatter(ArrayType &input_array, ArrayType const &updates,
             std::vector<SizeVector> const &indices)
{
  assert(indices.size() == updates.size());

  auto     indices_it = indices.begin();
  SizeType update_idx{0};

  while (indices_it != indices.end())
  {
    input_array.data()[input_array.ComputeIndex(*indices_it)] = updates[update_idx];
    ++indices_it;
    ++update_idx;
  }
}

/**
 * returns the product of all values in one array
 * @tparam ArrayType
 * @tparam T
 * @param array1
 * @param ret
 */
template <typename ArrayType, typename T, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Product(ArrayType const &array1, T &ret)
{
  if (array1.size() == 0)
  {
    ret = T{0};
  }
  else
  {
    ret = typename ArrayType::Type(1);
    for (auto const &val : array1)
    {
      ret *= val;
    }
  }
}

template <typename T, typename C /*template<class> class C*/,
          typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsMathArray<Tensor<T, C>, T> Product(Tensor<T, C> const &array1)
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
  if (obj1.empty())
  {
    ret = 0;
  }
  else
  {
    ret = std::accumulate(std::begin(obj1), std::end(obj1), T(1), std::multiplies<>());
  }
}
template <typename T>
T Product(std::vector<T> const &obj1)
{
  T ret;
  Product(obj1, ret);
  return ret;
}

/**
 * Finds the maximum value in an array
 * @tparam ArrayType
 * @tparam T
 * @param array
 * @param ret
 * @return
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Max(ArrayType const &array, typename ArrayType::Type &ret)
{
  ret = numeric_lowest<typename ArrayType::Type>();
  for (typename ArrayType::Type const &e : array)
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
  assert(axis < array.shape().size());

  if (array.shape().size() == 1)
  {
    assert(axis == 0);
    ret[0] = Max(array);
  }
  else
  {
    // Max along a single axis
    SizeType axis_length = array.shape()[axis];

    assert(axis_length > 1);
    assert(ret.size() == Divide(Product(array.shape()), array.shape()[axis]));

    // specialised implementation uses view if axis is the last dimension
    if (axis == (array.shape().size() - 1))
    {
      // fill the return with the first index values
      ret.Assign(array.View(0));

      //
      for (SizeType n{1}; n < axis_length; ++n)
      {
        auto cur_view    = array.View(n);
        auto cur_view_it = cur_view.begin();
        auto rit         = ret.begin();

        // check every element in the n-1 dimensional return
        while (cur_view_it.is_valid())
        {
          if (*cur_view_it > *rit)
          {
            *rit = *cur_view_it;
          }
          ++rit;
          ++cur_view_it;
        }
      }
    }
    else
    {
      // fill the return with the first index values
      ret.Assign(array.Slice(0, axis));

      //
      for (SizeType n{1}; n < axis_length; ++n)
      {
        auto cur_slice    = array.Slice(n, axis);
        auto cur_slice_it = cur_slice.cbegin();
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
 * Min function returns the smallest value in the array
 * @tparam ArrayType input array type
 * @tparam T input scalar return param
 * @param array input array
 * @param ret return value
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Min(ArrayType const &array, typename ArrayType::Type &ret)
{
  ret = numeric_max<typename ArrayType::Type>();
  for (typename ArrayType::Type const &e : array)
  {
    if (e < ret)
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
 * Implementation of Min that returns the n-1 dim array by finding the min of all 1-d vectors within
 * the array
 * @tparam ArrayType array or tensor type
 * @param array the array or tensor
 * @param axis the axis over which to iterate
 * @param ret the return object with n-1 dims
 */
template <typename ArrayType>
void Min(ArrayType const &array, typename ArrayType::SizeType const &axis, ArrayType &ret)
{
  assert(array.shape().size() <= 2);
  assert(axis < array.shape().size());

  if (array.shape().size() == 1)
  {
    assert(axis == 0);
    ret[0] = Min(array);
  }
  else
  {  // Argmax along a single axis
    SizeType axis_length = array.shape()[axis];
    assert(axis_length > 1);
    assert(ret.size() == Divide(Product(array.shape()), array.shape()[axis]));

    // specialisation uses View if axis == last dimension
    if (axis == array.shape().size() - 1)
    {
      // fill the return with the first index values
      ret.Assign(array.View(0));

      //
      for (SizeType n{1}; n < axis_length; ++n)
      {
        auto cur_view    = array.View(n);
        auto cur_view_it = cur_view.begin();
        auto rit         = ret.begin();

        // check every element in the n-1 dimensional return
        while (cur_view_it.is_valid())
        {
          if (*cur_view_it < *rit)
          {
            *rit = *cur_view_it;
          }
          ++rit;
          ++cur_view_it;
        }
      }
    }
    else
    {
      // fill the return with the first index values
      ret.Assign(array.Slice(0, axis));

      //
      for (SizeType n{1}; n < axis_length; ++n)
      {
        auto cur_slice    = array.Slice(n, axis);
        auto cur_slice_it = cur_slice.cbegin();
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
}

/**
 * Returns an array containing the elementwise maximum of two other arrays
 * @param x array input 1
 * @param y array input 2
 * @return the combined array
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Maximum(ArrayType const &array1, ArrayType const &array2,
                                             ArrayType &ret)
{
  assert(array1.shape() == array2.shape());
  assert(ret.shape() == array2.shape());

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
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Maximum(ArrayType const &array1, ArrayType const &array2)
{
  ArrayType ret(array1.shape());
  Maximum(array1, array2, ret);
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

/**
 * Sums cells of Tensor along given axis
 * @tparam ArrayType
 * @param obj1
 * @param axis Axis along which sum is computed
 * @param ret Output Tensor of shape of input with size 1 along given axis
 */
template <typename ArrayType>
void ReduceSum(ArrayType const &obj1, SizeType axis, ArrayType &ret)
{

  using DataType = typename ArrayType::Type;
  ret.Fill(static_cast<DataType>(0));

  Reduce(axis, [](DataType const &x, DataType &y) { y += x; }, obj1, ret);
}

/**
 * Sums cells of Tensor along given axis
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axis Axis along which sum is computed
 * @return  Output Tensor of shape of input with size 1 along given axis
 */
template <typename ArrayType>
ArrayType ReduceSum(ArrayType const &obj1, SizeType axis)
{
  SizeVector new_shape = obj1.shape();
  new_shape.at(axis)   = 1;
  ArrayType ret{new_shape};
  ReduceSum(obj1, axis, ret);
  return ret;
}

/**
 * Sums cells of Tensor along multiple given axes
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axes Vector of axes along which sum is computed
 * @param ret Output Tensor of shape of input with size 1 along given axes
 */
template <typename ArrayType>
void ReduceSum(ArrayType const &obj1, std::vector<SizeType> axes, ArrayType &ret)
{

  using DataType = typename ArrayType::Type;
  ret.Fill(static_cast<DataType>(0));

  Reduce(axes, [](DataType const &x, DataType &y) { y += x; }, obj1, ret);
}

/**
 * Sums cells of Tensor along multiple given axes
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axes Vector of axes along which sum is computed
 * @return Output Tensor of shape of input with size 1 along given axes
 */
template <typename ArrayType>
ArrayType ReduceSum(ArrayType const &obj1, std::vector<SizeType> axes)
{
  SizeVector new_shape = obj1.shape();

  for (SizeType i{0}; i < axes.size(); i++)
  {
    new_shape.at(axes.at(i)) = 1;
  }

  ArrayType ret{new_shape};
  ReduceSum(obj1, axes, ret);
  return ret;
}

/**
 * Average cells of Tensor along given axis
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axis Axis along which mean is computed
 * @param ret Output Tensor of shape of input with size 1 along given axis
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> ReduceMean(ArrayType const &                   obj1,
                                                typename ArrayType::SizeType const &axis,
                                                ArrayType &                         ret)
{
  using Type = typename ArrayType::Type;

  auto n = static_cast<Type>(obj1.shape().at(axis));
  ReduceSum(obj1, axis, ret);
  Divide(ret, n, ret);
}

/**
 * Average cells of Tensor along given axis
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axis Axis along which mean is computed
 * @return Output Tensor of shape of input with size 1 along given axis
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> ReduceMean(ArrayType const &                   obj1,
                                                     typename ArrayType::SizeType const &axis)
{
  using Type = typename ArrayType::Type;

  auto      n   = static_cast<Type>(obj1.shape().at(axis));
  ArrayType ret = ReduceSum(obj1, axis);
  Divide(ret, n, ret);
  return ret;
}

/**
 * Average cells of Tensor along multiple given axes
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axes Vector of axes along which mean is computed
 * @param ret Output Tensor of shape of input with size 1 along given axes
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> ReduceMean(ArrayType const &            obj1,
                                                std::vector<SizeType> const &axes, ArrayType &ret)
{
  using Type = typename ArrayType::Type;

  SizeType n{1};
  for (SizeType i{0}; i < axes.size(); i++)
  {
    n *= obj1.shape().at(axes.at(i));
  }

  ReduceSum(obj1, axes, ret);
  Divide(ret, static_cast<Type>(n), ret);
}

/**
 * Average cells of Tensor along multiple given axes
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axes Vector of axes along which mean is computed
 * @return Output Tensor of shape of input with size 1 along given axes
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> ReduceMean(ArrayType const &            obj1,
                                                     std::vector<SizeType> const &axes)
{
  using Type = typename ArrayType::Type;

  SizeType n{1};
  for (SizeType i{0}; i < axes.size(); i++)
  {
    n *= obj1.shape().at(axes.at(i));
  }

  Type ret = ReduceSum(obj1, axes);
  Divide(ret, static_cast<Type>(n), ret);
  return ret;
}

/**
 * Distance between max and min values in an array
 * @tparam ArrayType
 * @param array
 * @param ret
 */
template <typename ArrayType>
void PeakToPeak(ArrayType const &array, typename ArrayType::Type &ret)
{
  ret                          = numeric_lowest<typename ArrayType::Type>();
  typename ArrayType::Type min = numeric_max<typename ArrayType::Type>();
  auto                     it  = array.cbegin();
  while (it.is_valid())
  {
    if (*it > ret)
    {
      ret = *it;
    }
    if (*it < min)
    {
      min = *it;
    }
    ++it;
  }
  ret = ret - min;
}

template <typename ArrayType>
typename ArrayType::Type PeakToPeak(ArrayType const &array)
{
  typename ArrayType::Type ret;
  PeakToPeak(array, ret);
  return ret;
}

/**
 * Implementation of PeakToPeak that returns the n-1 dim array by finding the max-min of all 1-d
 * vectors within the array
 * @tparam ArrayType
 * @param array
 * @param axis
 * @param ret
 */
template <typename ArrayType>
void PeakToPeak(ArrayType const &array, typename ArrayType::SizeType const &axis, ArrayType &ret)
{
  assert(array.shape().size() <= 2);
  assert(axis < array.shape().size());

  if (array.shape().size() == 1)
  {
    assert(axis == 0);
    ret[0] = PeakToPeak(array);
  }
  else
  {  // Argmax-Argmin along a single axis
    SizeType axis_length = array.shape()[axis];
    assert(axis_length > 1);
    assert(ret.size() == Divide(Product(array.shape()), array.shape()[axis]));

    if (axis == array.shape().size() - 1)
    {
      // fill the return with the first index values
      ret.Assign(array.View(0));
      ArrayType min(ret.shape());
      min.Assign(array.View(0));

      //
      for (SizeType n{1}; n < axis_length; ++n)
      {
        auto cur_view    = array.View(n);
        auto cur_view_it = cur_view.begin();
        auto rit         = ret.begin();
        auto mit         = min.begin();

        // check every element in the n-1 dimensional return
        while (cur_view_it.is_valid())
        {
          if (*cur_view_it > *rit)
          {
            *rit = *cur_view_it;
          }
          if (*cur_view_it < *mit)
          {
            *mit = *cur_view_it;
          }
          ++rit;
          ++mit;
          ++cur_view_it;
        }
      }

      // i.e. ret=max-min, because max is stored in ret
      fetch::math::Subtract(ret, min, ret);
    }
    else
    {
      // fill the return with the first index values
      ret.Assign(array.Slice(0, axis));
      ArrayType min(ret.shape());
      min.Assign(array.Slice(0, axis));

      //
      for (SizeType n{1}; n < axis_length; ++n)
      {
        auto cur_slice    = array.Slice(n, axis);
        auto cur_slice_it = cur_slice.cbegin();
        auto rit          = ret.begin();
        auto mit          = min.begin();

        // check every element in the n-1 dimensional return
        while (cur_slice_it.is_valid())
        {
          if (*cur_slice_it > *rit)
          {
            *rit = *cur_slice_it;
          }
          if (*cur_slice_it < *mit)
          {
            *mit = *cur_slice_it;
          }
          ++rit;
          ++mit;
          ++cur_slice_it;
        }
      }

      // i.e. ret=max-min, because max is stored in ret
      fetch::math::Subtract(ret, min, ret);
    }
  }
}

template <typename ArrayType>
ArrayType PeakToPeak(ArrayType const &array, typename ArrayType::SizeType const &axis)
{
  ArrayType ret(Divide(Product(array.shape()), array.shape().at(axis)));
  PeakToPeak(array, axis, ret);
  return ret;
}

/**
 * Computes maximums of cells of Tensor along multiple given axis
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axis Axis along which max is computed
 * @param ret Output Tensor of shape of input with size 1 along given axis
 */
template <typename ArrayType>
void ReduceMax(ArrayType const &obj1, SizeType axis, ArrayType &ret)
{

  using DataType = typename ArrayType::Type;
  ret.Fill(numeric_lowest<DataType>());
  Reduce(axis, [](DataType const &x, DataType &y) { y = (x < y) ? y : x; }, obj1, ret);
}

/**
 * Computes maximums of cells of Tensor along multiple given axis
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axis Axis along which max is computed
 * @return Output Tensor of shape of input with size 1 along given axis
 */
template <typename ArrayType>
ArrayType ReduceMax(ArrayType const &obj1, SizeType axis)
{
  SizeVector new_shape = obj1.shape();
  new_shape.at(axis)   = 1;
  ArrayType ret{new_shape};
  ReduceMax(obj1, axis, ret);
  return ret;
}

/**
 * Computes maximums of cells of Tensor along multiple given axis
 * @tparam ArrayType
 * @param obj1 Input Tensor
 * @param axes Vector of axes along which max is computed
 * @param ret Output Tensor of shape of input with size 1 along given axes
 */
template <typename ArrayType>
void ReduceMax(ArrayType const &obj1, std::vector<SizeType> axes, ArrayType &ret)
{

  using DataType = typename ArrayType::Type;
  ret.Fill(numeric_lowest<DataType>());

  Reduce(axes, [](DataType const &x, DataType &y) { y = (x < y) ? y : x; }, obj1, ret);
}

/**
 * Computes maximums of cells of Tensor along multiple given axis
 * @tparam ArrayType
 * @param obj1
 * @param axes Vector of axes along which max is computed
 * @return Output Tensor of shape of input with size 1 along given axes
 */
template <typename ArrayType>
ArrayType ReduceMax(ArrayType const &obj1, std::vector<SizeType> axes)
{
  SizeVector new_shape = obj1.shape();

  for (SizeType i{0}; i < axes.size(); i++)
  {
    new_shape.at(axes.at(i)) = 1;
  }

  ArrayType ret{new_shape};
  ReduceMax(obj1, axes, ret);
  return ret;
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
    assert(ret.size() == SizeType(1));
    SizeType position = 0;
    auto     it       = array.begin();
    Type     value    = numeric_lowest<Type>();

    auto counter = SizeType{0};
    while (it.is_valid())
    {
      if (*it > value)
      {
        value    = *it;
        position = counter;
      }
      ++counter;
      ++it;
    }

    ret[0] = static_cast<Type>(position);
  }
  else
  {
    // Argmax along a single axis
    SizeType axis_length = array.shape()[axis];
    assert(axis_length > 1);
    assert(ret.size() == Divide(Product(array.shape()), array.shape()[axis]));

    ret.Fill(Type(0));

    // specialisation uses View if axis == last dimension
    if (axis == array.shape().size() - 1)
    {
      auto max_view = (array.View(0)).Copy();

      for (SizeType n{1}; n < axis_length; ++n)
      {
        auto cur_view = array.View(n);

        auto max_view_it = max_view.begin();
        auto cur_view_it = cur_view.begin();
        auto ret_it      = ret.begin();

        // check every element in the n-1 dimensional return
        while (max_view_it.is_valid())
        {
          if (*cur_view_it > *max_view_it)
          {
            *ret_it      = static_cast<Type>(n);
            *max_view_it = *cur_view_it;
          }
          ++ret_it;
          ++cur_view_it;
          ++max_view_it;
        }
      }
    }
    else
    {
      auto max_slice = (array.Slice(0, axis)).Copy();

      for (SizeType n{1}; n < axis_length; ++n)
      {
        auto cur_slice = array.Slice(n, axis);

        auto max_slice_it = max_slice.begin();
        auto cur_slice_it = cur_slice.cbegin();
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
  auto aview = A.View();
  auto bview = B.View();

  if (aview.width() != bview.height())
  {
    throw fetch::math::exceptions::WrongShape("expected A width to equal B height.");
  }

  if (ret.shape() != std::vector<SizeType>({aview.height(), bview.width()}))
  {
    ret.Resize({aview.height(), bview.width()});
  }

  using Type = typename ArrayType::Type;
  using namespace linalg;

  enum
  {
    OPTIMISATION_FLAGS = meta::HasVectorSupport<Type>::value
                             ? platform::Parallelisation::VECTORISE
                             : platform::Parallelisation::NOT_PARALLEL
  };

  Blas<Type, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * _A * _B + _beta * _C), OPTIMISATION_FLAGS>
      gemm_nn;

  gemm_nn(static_cast<Type>(1), aview, bview, static_cast<Type>(0), ret.View());
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
  auto aview = A.View();
  auto bview = B.View();

  if (aview.width() != bview.width())
  {
    throw exceptions::WrongShape("expected A and B to have same width.");
  }

  if (ret.shape() != std::vector<SizeType>({aview.height(), bview.width()}))
  {
    ret.Resize({aview.height(), bview.height()});
  }

  using Type = typename ArrayType::Type;
  using namespace linalg;

  enum
  {
    OPTIMISATION_FLAGS = meta::HasVectorSupport<Type>::value
                             ? platform::Parallelisation::VECTORISE
                             : platform::Parallelisation::NOT_PARALLEL
  };

  Blas<Type, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * _A * T(_B) + _beta * _C), OPTIMISATION_FLAGS>
      gemm_nt;

  gemm_nt(static_cast<Type>(1), A.View(), B.View(), static_cast<Type>(0), ret.View());
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
  auto aview = A.View();
  auto bview = B.View();

  if (aview.height() != bview.height())
  {
    throw exceptions::WrongShape("expected A and B to have same height.");
  }

  if (ret.shape() != std::vector<SizeType>({aview.height(), bview.width()}))
  {
    ret.Resize({aview.width(), bview.width()});
  }

  using Type = typename ArrayType::Type;
  using namespace linalg;

  enum
  {
    OPTIMISATION_FLAGS = meta::HasVectorSupport<Type>::value
                             ? platform::Parallelisation::VECTORISE
                             : platform::Parallelisation::NOT_PARALLEL
  };

  Blas<Type, Signature(_C <= _alpha, _A, _B, _beta, _C),
       Computes(_C <= _alpha * T(_A) * _B + _beta * _C), OPTIMISATION_FLAGS>
      gemm_tn;

  gemm_tn(static_cast<Type>(1), A.View(), B.View(), static_cast<Type>(0), ret.View());
}

template <class ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> TransposeDot(ArrayType const &A,
                                                                    ArrayType const &B)
{
  std::vector<typename ArrayType::SizeType> return_shape{A.shape().at(1), B.shape().at(1)};
  ArrayType                                 ret(return_shape);
  TransposeDot(A, B, ret);
  return ret;
}

template <typename ArrayType>
fetch::math::meta::IfIsMathArray<ArrayType, void> DynamicStitch(ArrayType &      input_array,
                                                                ArrayType const &indices,
                                                                ArrayType const &data)
{
  assert(data.size() <= input_array.size());
  assert(input_array.size() > static_cast<typename ArrayType::SizeType>(Max(indices)));
  assert(static_cast<typename ArrayType::SizeType>(Min(indices)) >= 0);
  input_array.Resize({indices.size()});

  auto ind_it  = indices.cbegin();
  auto data_it = data.cbegin();

  while (data_it.is_valid())
  {
    // loop through all output data locations identifying the next data point to copy into it
    input_array.Set(SizeType(*ind_it), *data_it);
    ++data_it;
    ++ind_it;
  }
}

}  // namespace math
}  // namespace fetch
