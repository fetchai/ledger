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

#include "math/kernels/approx_exp.hpp"
#include "math/kernels/approx_log.hpp"
#include "math/kernels/approx_logistic.hpp"
#include "math/kernels/approx_soft_max.hpp"
#include "math/kernels/basic_arithmetics.hpp"
#include "math/kernels/relu.hpp"
#include "math/kernels/sign.hpp"
#include "math/kernels/standard_functions.hpp"

#include "core/assert.hpp"
#include "core/meta/type_traits.hpp"
#include "math/ndarray_broadcast.hpp"
#include "vectorise/memory/range.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>

/////////////////////////////////
/// include specific math functions
/////////////////////////////////

#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/free_functions/standard_functions/abs.hpp"
#include "math/free_functions/standard_functions/exp.hpp"
#include "math/free_functions/standard_functions/fmod.hpp"
#include "math/free_functions/standard_functions/log.hpp"
#include "math/free_functions/standard_functions/remainder.hpp"

/////////////////////////////////
/// blas libraries
/////////////////////////////////

#include "math/linalg/blas/gemm_nn_vector.hpp"
#include "math/linalg/blas/gemm_nn_vector_threaded.hpp"
#include "math/linalg/blas/gemm_nt_vector.hpp"
#include "math/linalg/blas/gemm_nt_vector_threaded.hpp"
#include "math/linalg/blas/gemm_tn_vector.hpp"
#include "math/linalg/blas/gemm_tn_vector_threaded.hpp"

#include "math/meta/type_traits.hpp"

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

namespace linalg {
template <typename T, typename C, typename S>
class Matrix;
}

template <typename T, typename C>
T Max(ShapeLessArray<T, C> const &array);

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
void Scatter(ShapeLessArray<T, C> &input_array, ShapeLessArray<T, C> const &updates,
             ShapeLessArray<T, C> const &indices)
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
void Transpose(NDArray<T, C> &input_array, NDArray<T, C> const &perm)
{
  assert(perm.size() == input_array.shape().size());
}

/**
 * Efficient vectorised and threaded routine for C = A.T(B)
 * @param A
 * @param B
 * @return
 */
template <typename T, typename C>
void Dot(NDArray<T, C> const &A, NDArray<T, C> const &B, NDArray<T, C> &ret, T alpha = 1.0,
         T beta = 0.0, bool threaded = false)
{
  assert(ret.shape().size() == 2);
  ret.Resize(A.shape()[0] * B.shape()[0]);

  if (threaded)
  {
    linalg::Blas<T, NDArray<T, C>,
                 Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta,
                           linalg::_C),
                 Computes(linalg::_C = linalg::_alpha * linalg::_A * linalg::_B +
                                       linalg::_beta * linalg::_C),
                 platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
        gemm_nn_vector_threaded;

    gemm_nn_vector_threaded(alpha, A, B, beta, ret);
  }
  else
  {
    linalg::Blas<T, NDArray<T, C>,
                 Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta,
                           linalg::_C),
                 Computes(linalg::_C = linalg::_alpha * linalg::_A * linalg::_B +
                                       linalg::_beta * linalg::_C),
                 platform::Parallelisation::VECTORISE>
        gemm_nn_vector;

    gemm_nn_vector(alpha, A, B, beta, ret);
  }
}
template <typename T, typename C>
NDArray<T, C> Dot(NDArray<T, C> const &A, NDArray<T, C> const &B, bool threaded = false)
{
  std::vector<std::size_t> return_shape{A.shape()[0], B.shape()[0]};
  NDArray<T, C>            ret(return_shape);
  Dot(A, B, ret, threaded);
  return ret;
}
template <typename T, typename C, typename S>
void Dot(linalg::Matrix<T, C, S> const &A, linalg::Matrix<T, C, S> const &B,
         linalg::Matrix<T, C, S> &ret, T alpha = 1.0, T beta = 0.0, bool threaded = false)
{
  ret.Resize(A.shape()[0], B.shape()[1]);

  if (threaded)
  {
    linalg::Blas<T, linalg::Matrix<T, C, S>,
                 Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta,
                           linalg::_C),
                 Computes(linalg::_C = linalg::_alpha * linalg::_A * linalg::_B +
                                       linalg::_beta * linalg::_C),
                 platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
        gemm_nn_vector_threaded;

    gemm_nn_vector_threaded(alpha, A, B, beta, ret);
  }
  else
  {
    linalg::Blas<T, linalg::Matrix<T, C, S>,
                 Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta,
                           linalg::_C),
                 Computes(linalg::_C = linalg::_alpha * linalg::_A * linalg::_B +
                                       linalg::_beta * linalg::_C),
                 platform::Parallelisation::VECTORISE>
        gemm_nn_vector;

    gemm_nn_vector(alpha, A, B, beta, ret);
  }
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Dot(linalg::Matrix<T, C, S> const &A, linalg::Matrix<T, C, S> const &B,
                            bool threaded = false)
{
  std::vector<std::size_t> return_shape{A.shape()[0], B.shape()[1]};
  linalg::Matrix<T, C, S>  ret(return_shape);
  Dot(A, B, ret, 1.0, 0.0, threaded);
  return ret;
}

/**
 * Efficient vectorised and threaded routine for C = A.T(B)
 * @param A
 * @param B
 * @return
 */
template <typename ArrayType>
fetch::math::meta::IsMathShapeArrayLike<ArrayType, void> DotTranspose(
    ArrayType const &A, ArrayType const &B, ArrayType &ret, typename ArrayType::Type alpha = 1.0,
    typename ArrayType::Type beta = 0.0, bool threaded = false)
{
  ret.Resize(A.shape()[0], B.shape()[0]);

  if (threaded)
  {
    linalg::Blas<
        typename ArrayType::Type, ArrayType,
        Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta, linalg::_C),
        Computes(linalg::_C = linalg::_alpha * linalg::_A * fetch::math::linalg::T(linalg::_B) +
                              linalg::_beta * linalg::_C),
        platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
        gemm_nt_vector_threaded;

    gemm_nt_vector_threaded(alpha, A, B, beta, ret);
  }
  else
  {
    linalg::Blas<
        typename ArrayType::Type, ArrayType,
        Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta, linalg::_C),
        Computes(linalg::_C = linalg::_alpha * linalg::_A * fetch::math::linalg::T(linalg::_B) +
                              linalg::_beta * linalg::_C),
        platform::Parallelisation::VECTORISE>
        gemm_nt_vector;

    gemm_nt_vector(alpha, A, B, beta, ret);
  }
}
template <typename ArrayType>
fetch::math::meta::IsMathShapeArrayLike<ArrayType, ArrayType> DotTranspose(
    ArrayType const &A, ArrayType const &B, typename ArrayType::Type alpha = 1.0,
    typename ArrayType::Type beta = 0.0, bool threaded = false)
{

  std::vector<std::size_t> return_shape{A.shape()[0], B.shape()[0]};
  ArrayType                ret(return_shape);

  DotTranspose(A, B, ret, alpha, beta, threaded);

  return ret;
}
template <typename ArrayType>
fetch::math::meta::IsMathShapeArrayLike<ArrayType, ArrayType> DotTranspose(ArrayType const &A,
                                                                           ArrayType const &B,
                                                                           bool threaded = false)
{

  std::vector<std::size_t> return_shape{A.shape()[0], B.shape()[0]};
  ArrayType                ret(return_shape);

  DotTranspose(A, B, ret, 1.0, 0.0, threaded);

  return ret;
}

/**
 * Efficient vectorised and threaded routine for C = T(A).B
 * @param A
 * @param B
 * @return
 */
template <typename T, typename C>
void TransposeDot(NDArray<T, C> const &A, NDArray<T, C> const &B, NDArray<T, C> &ret, T alpha = 1.0,
                  T beta = 0.0, bool threaded = false)
{
  assert(ret.shape().size() == 2);
  std::vector<std::size_t> return_shape{A.shape()[1], B.shape()[1]};
  ret.Reshape(return_shape);

  if (threaded)
  {
    linalg::Blas<
        T, NDArray<T, C>,
        Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta, linalg::_C),
        Computes(linalg::_C = linalg::_alpha * fetch::math::linalg::T(linalg::_A) * linalg::_B +
                              linalg::_beta * linalg::_C),
        platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
        gemm_tn_vector_threaded;

    gemm_tn_vector_threaded(alpha, A, B, beta, ret);
  }
  else
  {
    linalg::Blas<
        T, NDArray<T, C>,
        Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta, linalg::_C),
        Computes(linalg::_C = linalg::_alpha * fetch::math::linalg::T(linalg::_A) * linalg::_B +
                              linalg::_beta * linalg::_C),
        platform::Parallelisation::VECTORISE>
        gemm_tn_vector;

    gemm_tn_vector(alpha, A, B, beta, ret);
  }
}
template <typename T, typename C>
NDArray<T, C> TransposeDot(NDArray<T, C> const &A, NDArray<T, C> const &B, T alpha = 1.0,
                           T beta = 0.0, bool threaded = false)
{
  assert(A.shape().size() == 2);
  assert(B.shape().size() == 2);
  std::vector<std::size_t> return_shape{A.shape()[1], B.shape()[1]};
  NDArray<T, C>            ret(return_shape);

  TransposeDot(A, B, ret, alpha, beta, threaded);

  return ret;
}
template <typename T, typename C>
NDArray<T, C> TransposeDot(NDArray<T, C> const &A, NDArray<T, C> const &B, bool threaded = false)
{
  assert(A.shape().size() == 2);
  assert(B.shape().size() == 2);
  std::vector<std::size_t> return_shape{A.shape()[1], B.shape()[1]};
  NDArray<T, C>            ret(return_shape);

  TransposeDot(A, B, ret, 1.0, 0.0, threaded);

  return ret;
}

template <typename T, typename C, typename S>
void TransposeDot(linalg::Matrix<T, C, S> const &A, linalg::Matrix<T, C, S> const &B,
                  linalg::Matrix<T, C, S> &ret, T alpha = 1.0, T beta = 0.0, bool threaded = false)
{
  ret.Resize(A.width(), B.width());

  if (threaded)
  {
    linalg::Blas<
        T, linalg::Matrix<T, C, S>,
        Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta, linalg::_C),
        Computes(linalg::_C = linalg::_alpha * fetch::math::linalg::T(linalg::_A) * linalg::_B +
                              linalg::_beta * linalg::_C),
        platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
        gemm_tn_vector_threaded;

    gemm_tn_vector_threaded(alpha, A, B, beta, ret);
  }
  else
  {
    linalg::Blas<
        T, linalg::Matrix<T, C, S>,
        Signature(linalg::_C <= linalg::_alpha, linalg::_A, linalg::_B, linalg::_beta, linalg::_C),
        Computes(linalg::_C = linalg::_alpha * fetch::math::linalg::T(linalg::_A) * linalg::_B +
                              linalg::_beta * linalg::_C),
        platform::Parallelisation::VECTORISE>
        gemm_tn_vector;

    gemm_tn_vector(alpha, A, B, beta, ret);
  }
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> TransposeDot(linalg::Matrix<T, C, S> const &A,
                                     linalg::Matrix<T, C, S> const &B, T alpha = 1.0, T beta = 0.0,
                                     bool threaded = false)
{
  std::vector<std::size_t> return_shape{A.shape()[1], B.shape()[1]};
  linalg::Matrix<T, C, S>  ret(return_shape);

  TransposeDot(A, B, ret, alpha, beta, threaded);

  return ret;
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> TransposeDot(linalg::Matrix<T, C, S> const &A,
                                     linalg::Matrix<T, C, S> const &B, bool threaded = false)
{
  std::vector<std::size_t> return_shape{A.shape()[1], B.shape()[1]};
  linalg::Matrix<T, C, S>  ret(return_shape);

  TransposeDot(A, B, ret, 1.0, 0.0, threaded);

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
void Concat(ShapeLessArray<T, C> &ret, std::vector<ShapeLessArray<T, C>> const &input_arrays)
{
  details::ConcatImplementation(input_arrays, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Concat(std::vector<ShapeLessArray<T, C>> const &input_arrays)
{
  ShapeLessArray<T, C> ret;
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
NDArray<T, C> Concat(std::vector<NDArray<T, C>> input_arrays, std::size_t const &axis)
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
void DynamicStitch(ShapeLessArray<T, C> &input_array, ShapeLessArray<T, C> const &indices,
                   ShapeLessArray<T, C> const &data)
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
void BooleanMask(ShapeLessArray<T, C> &input_array, ShapeLessArray<T, C> const &mask,
                 ShapeLessArray<T, C> &ret)
{
  details::BooleanMaskImplementation(input_array, mask, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> BooleanMask(ShapeLessArray<T, C> &      input_array,
                                 ShapeLessArray<T, C> const &mask)
{
  ShapeLessArray<T, C> ret;
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

/**
 * raise 2 to power input values of x
 * @param x
 */
template <typename ArrayType>
void Exp2(ArrayType &x)
{
  kernels::stdlib::Exp2<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * exp(x) - 1
 * @param x
 */
template <typename ArrayType>
void Expm1(ArrayType &x)
{
  kernels::stdlib::Expm1<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * natural logarithm of x
 * @param x
 */
template <typename ArrayType>
void Log10(ArrayType &x)
{
  kernels::stdlib::Log10<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * log base 2
 * @param x
 */
template <typename ArrayType>
void Log2(ArrayType &x)
{
  kernels::stdlib::Log2<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * natural log 1 + x
 * @param x
 */
template <typename ArrayType>
void Log1p(ArrayType &x)
{
  kernels::stdlib::Log1p<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * square root
 * @param x
 */
template <typename ArrayType>
void Sqrt(ArrayType &x)
{
  kernels::stdlib::Sqrt<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * cubic root x
 * @param x
 */
template <typename ArrayType>
void Cbrt(ArrayType &x)
{
  kernels::stdlib::Cbrt<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * raise to power
 * @param x
 */
template <typename ArrayType>
void Pow(ArrayType &x)
{
  kernels::stdlib::Pow<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * square
 * @param x
 */
template <typename ArrayType>
void Square(ArrayType &x)
{
  for (std::size_t i = 0; i < x.size(); ++i)
  {
    x[i] = x[i] * x[i];
  }
}
template <typename ArrayType>
void Square(ArrayType const &x, ArrayType &ret)
{
  for (std::size_t i = 0; i < x.size(); ++i)
  {
    ret[i] = x[i];
    ret[i] = ret[i] * ret[i];
  }
}

/**
 * sine of x
 * @param x
 */
template <typename ArrayType>
void Sin(ArrayType &x)
{
  kernels::stdlib::Sin<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * cosine of x
 * @param x
 */
template <typename ArrayType>
void Cos(ArrayType &x)
{
  kernels::stdlib::Cos<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * tangent of x
 * @param x
 */
template <typename ArrayType>
void Tan(ArrayType &x)
{
  kernels::stdlib::Tan<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc sine of x
 * @param x
 */
template <typename ArrayType>
void Asin(ArrayType &x)
{
  kernels::stdlib::Asin<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc cosine of x
 * @param x
 */
template <typename ArrayType>
void Acos(ArrayType &x)
{
  kernels::stdlib::Acos<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc tangent of x
 * @param x
 */
template <typename ArrayType>
void Atan(ArrayType &x)
{
  kernels::stdlib::Atan<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * arc tangent of x
 * @param x
 */
template <typename ArrayType>
void Atan2(ArrayType &x)
{
  kernels::stdlib::Atan2<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic sine of x
 * @param x
 */
template <typename ArrayType>
void Sinh(ArrayType &x)
{
  kernels::stdlib::Sinh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic cosine of x
 * @param x
 */
template <typename ArrayType>
void Cosh(ArrayType &x)
{
  kernels::stdlib::Cosh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic tangent of x
 * @param x
 */
template <typename ArrayType>
void Tanh(ArrayType &x)
{
  kernels::stdlib::Tanh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic arc sine of x
 * @param x
 */
template <typename ArrayType>
void Asinh(ArrayType &x)
{
  kernels::stdlib::Asinh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic arc cosine of x
 * @param x
 */
template <typename ArrayType>
void Acosh(ArrayType &x)
{
  kernels::stdlib::Acosh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * hyperbolic arc tangent of x
 * @param x
 */
template <typename ArrayType>
void Atanh(ArrayType &x)
{
  kernels::stdlib::Atanh<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * error function of x
 * @param x
 */
template <typename ArrayType>
void Erf(ArrayType &x)
{
  kernels::stdlib::Erf<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * complementary error function of x
 * @param x
 */
template <typename ArrayType>
void Erfc(ArrayType &x)
{
  kernels::stdlib::Erfc<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * factorial of x-1
 * @param x
 */
template <typename ArrayType>
void Tgamma(ArrayType &x)
{
  kernels::stdlib::Tgamma<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * log of factorial of x-1
 * @param x
 */
template <typename ArrayType>
void Lgamma(ArrayType &x)
{
  kernels::stdlib::Lgamma<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * ceiling round
 * @param x
 */
template <typename ArrayType>
void Ceil(ArrayType &x)
{
  kernels::stdlib::Ceil<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * floor rounding
 * @param x
 */
template <typename ArrayType>
void Floor(ArrayType &x)
{
  kernels::stdlib::Floor<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round towards 0
 * @param x
 */
template <typename ArrayType>
void Trunc(ArrayType &x)
{
  kernels::stdlib::Trunc<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in int format
 * @param x
 */
template <typename ArrayType>
void Round(ArrayType &x)
{
  kernels::stdlib::Round<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format
 * @param x
 */
template <typename ArrayType>
void Lround(ArrayType &x)
{
  kernels::stdlib::Lround<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format with long long return
 * @param x
 */
template <typename ArrayType>
void Llround(ArrayType &x)
{
  kernels::stdlib::Llround<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format
 * @param x
 */
template <typename ArrayType>
void Nearbyint(ArrayType &x)
{
  kernels::stdlib::Nearbyint<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int
 * @param x
 */
template <typename ArrayType>
void Rint(ArrayType &x)
{
  kernels::stdlib::Rint<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Lrint(ArrayType &x)
{
  kernels::stdlib::Lrint<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Llrint(ArrayType &x)
{
  kernels::stdlib::Llrint<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * finite check
 * @param x
 */
template <typename ArrayType>
void Isfinite(ArrayType &x)
{
  kernels::stdlib::Isfinite<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for inf values
 * @param x
 */
template <typename ArrayType>
void Isinf(ArrayType &x)
{
  kernels::stdlib::Isinf<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * checks for nans
 * @param x
 */
template <typename ArrayType>
void Isnan(ArrayType &x)
{
  kernels::stdlib::Isnan<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Hypot(ArrayType &x)
{
  kernels::stdlib::Hypot<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Frexp(ArrayType &x)
{
  kernels::stdlib::Frexp<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Ldexp(ArrayType &x)
{
  kernels::stdlib::Ldexp<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Modf(ArrayType &x)
{
  kernels::stdlib::Modf<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Scalbn(ArrayType &x)
{
  kernels::stdlib::Scalbn<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Scalbln(ArrayType &x)
{
  kernels::stdlib::Scalbln<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Ilogb(ArrayType &x)
{
  kernels::stdlib::Ilogb<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Logb(ArrayType &x)
{
  kernels::stdlib::Logb<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Nextafter(ArrayType &x)
{
  kernels::stdlib::Nextafter<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Nexttoward(ArrayType &x)
{
  kernels::stdlib::Nexttoward<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Copysign(ArrayType &x)
{
  kernels::stdlib::Copysign<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Fpclassify(ArrayType &x)
{
  kernels::stdlib::Fpclassify<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Isnormal(ArrayType &x)
{
  kernels::stdlib::Isnormal<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Signbit(ArrayType &x)
{
  kernels::stdlib::Signbit<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Isgreater(ArrayType &x)
{
  kernels::stdlib::Isgreater<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Isgreaterequal(ArrayType const &x, ArrayType const &y, ArrayType &z)
{
  kernels::stdlib::Isgreaterequal<typename ArrayType::Type> kernel;
  z.data().in_parallel().Apply(kernel, x.data(), y.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Isless(ArrayType &x)
{
  kernels::stdlib::Isless<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Islessequal(ArrayType &x)
{
  kernels::stdlib::Islessequal<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Islessgreater(ArrayType &x)
{
  kernels::stdlib::Islessgreater<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Isunordered(ArrayType &x)
{
  kernels::stdlib::Isunordered<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void ApproxExp(ArrayType &x)
{
  kernels::ApproxExp<typename ArrayType::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void ApproxLog(ArrayType &x)
{
  kernels::ApproxLog<typename ArrayType::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void ApproxLogistic(ArrayType &x)
{
  kernels::ApproxLogistic<typename ArrayType::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * rectified linear activation function
 * @param x
 */
template <typename ArrayType>
void Relu(ArrayType &x)
{
  kernels::Relu<typename ArrayType::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * replaces data with the sign (1 or -1)
 * @param x
 */
template <typename ArrayType>
void Sign(ArrayType &x)
{
  kernels::Sign<typename ArrayType::vector_register_type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

template <typename T, typename C, typename S>
void ReduceSum(linalg::Matrix<T, C, S> const &obj1, std::size_t axis, linalg::Matrix<T, C, S> &ret)
{
  assert((axis == 0) || (axis == 1));
  std::vector<std::size_t> access_idx{0, 0};
  if (axis == 0)
  {
    assert(ret.size() == obj1.width());
    for (std::size_t i = 0; i < ret.size(); ++i)
    {
      ret[i] = 0;
      for (std::size_t j = 0; j < obj1.shape()[0]; ++j)
      {
        ret[i] += obj1(j, i);
      }
    }
  }
  else
  {
    assert(ret.size() == obj1.height());
    for (std::size_t i = 0; i < ret.size(); ++i)
    {
      ret[i] = 0;
      for (std::size_t j = 0; j < obj1.shape()[1]; ++j)
      {
        ret[i] += obj1(i, j);
      }
    }
  }
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceSum(linalg::Matrix<T, C, S> const &obj1,
                                  linalg::Matrix<T, C, S> const &axis)
{
  assert(axis.shape()[0] == 1);
  assert(axis.shape()[1] == 1);
  return ReduceSum(obj1, std::size_t(axis[0]));
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceSum(linalg::Matrix<T, C, S> const &obj1, std::size_t axis)
{
  assert((axis == 0) || (axis == 1));
  if (axis == 0)
  {
    std::vector<std::size_t> new_shape{1, obj1.width()};
    linalg::Matrix<T, C, S>  ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
  else
  {
    std::vector<std::size_t> new_shape{obj1.height(), 1};
    linalg::Matrix<T, C, S>  ret{new_shape};
    ReduceSum(obj1, axis, ret);
    return ret;
  }
}

template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceSumImpl(linalg::Matrix<T, C, S> const &obj1, std::size_t const &axis)
{
  if (obj1.shape()[0] == 1)
  {
    return obj1;
  }
  else
  {
    return ReduceSumImpl(ReduceSum(obj1, axis), axis - 1);
  }
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceSum(linalg::Matrix<T, C, S> const &obj1)
{
  std::size_t axis = obj1.shape().size() - 1;
  //  linalg::Matrix<T, C, S> ret = ReduceSum(obj1, axis);

  return ReduceSumImpl(obj1, axis);
}

template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ReduceMean(linalg::Matrix<T, C, S> const &obj1, std::size_t const &axis)
{
  T n = obj1.shape()[0];
  return Divide(ReduceSum(obj1, axis), n);
}

template <typename ArrayType>
typename ArrayType::Type L2Norm(ArrayType const &A, ArrayType &ret)
{
  assert(A.size() == ret.size());
  assert(A.shape() == ret.shape());

  Square(A, ret);
  return std::sqrt(Sum(ret));
}
template <typename ArrayType>
typename ArrayType::Type L2Norm(ArrayType const &A)
{
  ArrayType ret{A.shape()};
  return L2Norm(A, ret);
}

template <typename ArrayType>
ArrayType MeanSquareError(ArrayType const &A, ArrayType const &B)
{
  assert(A.shape() == B.shape());
  ArrayType ret(A.shape());
  Subtract(A, B, ret);
  Square(ret);
  ret = ReduceSum(ret, 0);

  ret = Divide(ret, typename ArrayType::Type(A.shape()[0]));
  ret = Divide(ret, typename ArrayType::Type(
                        2));  // division by 2 allows us to cancel out with a 2 in the derivative
  return ret;
}

/**
 * Cross entropy loss with x as the prediction, and y as the ground truth
 * @tparam ArrayType
 * @param x a 2d array with axis 0 = examples, and axis 1 = dimension in prediction space
 * @param y same size as x with the correct predictions set to 1 in axis 1 and all other positions =
 * 0
 * @return
 */
template <typename ArrayType>
ArrayType CrossEntropyLoss(ArrayType const &x, ArrayType const &y)
{
  assert(x.shape() == y.shape());

  // we can't handle taking log(0), and the user should ensure this is never asked for
  // if in doubt the user can always call SoftmaxCrossEntropyLoss instead
  for (std::size_t k = 0; k < x.size(); ++k)
  {
    assert(x.At(k) != 0);
  }

  ArrayType logx{x.shape()};
  logx.Copy(x);
  Log(logx);

  ArrayType plogx{logx.shape()};
  for (std::size_t i = 0; i < logx.shape()[0]; ++i)
  {
    for (std::size_t j = 0; j < logx.shape()[1]; ++j)
    {
      if (y.At(i, j) == 0)
      {
        plogx.Set(i, j, 0);
      }
      else if (logx.At(i, j) == 0)
      {
        plogx.Set(i, j, 0);
      }
      else
      {
        plogx.Set(i, j, logx.At(i, j) * y.At(i, j));
      }
    }
  }

  auto                     cel      = Multiply(plogx, -1.0);
  typename ArrayType::Type n        = typename ArrayType::Type(cel.shape()[0]);
  auto                     mean_cel = ReduceSum(cel, 0);

  return Divide(mean_cel, n);
}

/**
 * Cross entropy loss with x as the prediction, and y as the ground truth
 * @tparam ArrayType
 * @param x a 2d array with axis 0 = examples, and axis 1 = dimension in prediction space
 * @param y same size as x with the correct predictions set to 1 in axis 1 and all other positions =
 * 0
 * @return Returns an Array of size 1 containing the loss value
 */
template <typename ArrayType>
ArrayType SoftmaxCrossEntropyLoss(ArrayType const &x, ArrayType const &y)
{
  assert(x.shape() == y.shape());
  assert(x.shape().size() == 2);

  auto n_examples = x.shape()[0];

  ArrayType sce_x{x.shape()};
  sce_x.Copy(x);

  // we don't explicitly call softmax, because we assume softmax was already included in the graph
  // (i.e. x is the output
  //  of softmax layer)

  auto      gt = ArgMax(y, 1);
  ArrayType log_likelihood{1};
  log_likelihood[0] = 0;

  for (std::size_t idx = 0; idx < n_examples; ++idx)
  {
    sce_x.Set(idx, static_cast<std::size_t>(gt[idx]),
              std::log(sce_x.At(idx, static_cast<std::size_t>(gt[idx]))));
    log_likelihood[0] -= sce_x.At(idx, static_cast<std::size_t>(gt[idx]));
  }

  return Divide(log_likelihood, static_cast<typename ArrayType::Type>(n_examples));
}

/**
 * The sigmoid function
 * @tparam ArrayType
 * @tparam T
 * @param y
 * @param y_hat
 * @param ret
 */
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Sigmoid(linalg::Matrix<T, C, S> const &A)
{
  linalg::Matrix<T, C, S> ret{A.shape()};
  ret.Copy(A);
  //  ret.data() = A.data().copy();

  Multiply(-1.0, ret, ret);
  Exp(ret);
  Add(ret, 1.0, ret);
  Divide(1.0, ret, ret);

  return ret;
}

template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Tanh(linalg::Matrix<T, C, S> const &A)
{
  linalg::Matrix<T, C, S> ret{A.shape()};
  ret.Copy(A);
  Multiply(2.0, ret, ret);
  Sigmoid(ret);
  Multiply(2.0, ret, ret);
  Subtract(ret, 1.0, ret);

  return ret;
}

/**
 * Max function for two values
 * @tparam T
 * @param datum1
 * @param datum2
 * @return
 */
template <typename T>
T Max(T const &datum1, T const &datum2, T &ret)
{
  ret = std::max(datum1, datum2);
  return ret;
}
template <typename T>
T Max(T const &datum1, T const &datum2)
{
  T ret{};
  ret = Max(datum1, datum2, ret);
  return ret;
}

/**
 * Finds the maximum value in an array
 * @tparam T data type
 * @tparam C container type
 * @param array array to search for maximum value
 * @return
 */
template <typename T, typename C>
T Max(ShapeLessArray<T, C> const &array, T &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
  return ret;
}
template <typename T, typename C>
T Max(ShapeLessArray<T, C> const &array)
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
inline void Max(ShapeLessArray<T, C> const &array, memory::Range r, T &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  if (r.is_trivial())
  {
    ret = array.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
  }
  else
  {  // non-trivial range is not vectorised
    typename ShapeLessArray<T, C>::Type ret =
        -std::numeric_limits<typename ShapeLessArray<T, C>::Type>::max();
    for (auto i : array)
    {
      ret = std::max(ret, i);
    }
  }
}

/**
 * Finds the maximum value in each row/column depending on axis and stores the output in ret
 * @tparam T
 * @tparam C
 * @tparam S
 * @param array the array to find max over
 * @param axis the axis along which to max
 * @param ret the return array
 */
template <typename T, typename C, typename S>
void Max(linalg::Matrix<T, C, S> const &array, std::size_t const &axis,
         linalg::Matrix<T, C, S> &ret)
{
  assert(axis == 0 || axis == 1);

  if (axis == 0)
  {
    assert(ret.shape()[0] == 1);
    assert(ret.shape()[1] == array.shape()[1]);
    for (std::size_t i = 0; i < array.shape()[1]; ++i)
    {
      ret.Set(0, i, -std::numeric_limits<typename linalg::Matrix<T, C, S>::Type>::max());
      for (std::size_t j = 0; j < array.shape()[0]; ++j)
      {
        ret.Set(0, i, std::max(ret.At(0, i), array.At(j, i)));
      }
    }
  }
  else
  {
    assert(ret.shape()[0] == array.shape()[0]);
    assert(ret.shape()[1] == 1);
    for (std::size_t i = 0; i < array.shape()[0]; ++i)
    {
      ret.Set(i, 0, -std::numeric_limits<typename linalg::Matrix<T, C, S>::Type>::max());
      for (std::size_t j = 0; j < array.shape()[1]; ++j)
      {
        ret.Set(i, 0, std::max(ret.At(i, 0), array.At(i, j)));
      }
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
 * Finds the argument of the maximum value in an array
 * @tparam T data type
 * @tparam C container type
 * @param array array to search for maximum value
 * @return
 */
template <typename T, typename C>
void ArgMax(ShapeLessArray<T, C> const &array, T &ret)
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
T ArgMax(ShapeLessArray<T, C> const &array)
{
  T ret;
  return ArgMax(array, ret);
}

template <typename T, typename C, typename S>
void ArgMax(linalg::Matrix<T, C, S> const &array, std::size_t axis, ShapeLessArray<T, C> &ret)
{
  assert(axis < 2);

  if (axis == 0)
  {
    assert(ret.size() == array.width());
    T cur_maxval;

    // just using ret as a free variable to store the current maxval for the loop here
    for (std::size_t i = 0; i < array.width(); ++i)
    {
      cur_maxval = std::numeric_limits<T>::lowest();
      for (std::size_t j = 0; j < array.height(); ++j)
      {
        if (cur_maxval < array.At(j, i))
        {
          ret[i]     = static_cast<T>(j);
          cur_maxval = array.At(j, i);
        }
      }
    }
  }
  else
  {
    assert(ret.size() == array.height());
    T cur_maxval;

    // just using ret as a free variable to store the current maxval for the loop here
    for (std::size_t i = 0; i < array.height(); ++i)
    {
      cur_maxval = std::numeric_limits<T>::lowest();
      for (std::size_t j = 0; j < array.width(); ++j)
      {
        if (cur_maxval < array.At(i, j))
        {
          ret[i]     = static_cast<T>(j);
          cur_maxval = array.At(i, j);
        }
      }
    }
  }
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> ArgMax(linalg::Matrix<T, C, S> const &array, std::size_t axis)
{
  assert(array.shape().size() == 2);
  assert(axis == 0 || axis == 1);

  if (axis == 0)
  {
    linalg::Matrix<T, C, S> ret{array.shape()[1]};
    ArgMax(array, axis, ret);
    return ret;
  }
  else
  {
    linalg::Matrix<T, C, S> ret{array.shape()[0]};
    ArgMax(array, axis, ret);
    return ret;
  }
}

/**
 * Min function for two values
 * @tparam T
 * @param datum1
 * @param datum2
 * @return
 */
template <typename T>
inline void Min(T const &datum1, T const &datum2, T &ret)
{
  ret = std::min(datum1, datum2);
}

/**
 * Min function for returning the smallest value in an array
 * @tparam ArrayType
 * @param array
 * @return
 */
template <typename T, typename C>
inline void Min(ShapeLessArray<T, C> const &array, T &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

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
inline void Min(ShapeLessArray<T, C> const &array, memory::Range r, T &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

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
 * softmax over all data in shapelessarray
 * @tparam T type
 * @tparam C container_type
 * @param array original data upon which to call softmax
 * @param ret new data with softmax applied
 */
namespace details {
template <typename ArrayType>
void SoftmaxImplementation(ArrayType const &array, ArrayType &ret)
{
  assert(ret.size() == array.size());

  // by subtracting the max we improve numerical stability, and the result will be identical
  std::vector<std::size_t> arr_shape{array.shape()[0], 1};
  ArrayType                array_max{arr_shape};
  ArrayType                array_sum{arr_shape};

  Max(array, 1, array_max);
  Subtract(array, array_max, ret);
  Exp(ret);

  ReduceSum(ret, 1, array_sum);
  Divide(ret, array_sum, ret);
}
}  // namespace details
template <typename T, typename C>
void Softmax(ShapeLessArray<T, C> &array, ShapeLessArray<T, C> &ret)
{
  assert(ret.size() == array.size());
  details::SoftmaxImplementation(array, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Softmax(ShapeLessArray<T, C> &array)
{
  ShapeLessArray<T, C> ret{array.size()};
  Softmax(array, ret);
  return ret;
}
template <typename T, typename C>
void Softmax(NDArray<T, C> const &array, NDArray<T, C> &ret)
{
  assert(ret.size() == array.size());
  ret.LazyReshape(array.shape());
  details::SoftmaxImplementation(array, ret);
}
template <typename T, typename C>
NDArray<T, C> Softmax(NDArray<T, C> const &array)
{
  NDArray<T, C> ret{array.shape()};
  Softmax(array, ret);
  return ret;
}
template <typename T, typename C, typename S>
void Softmax(linalg::Matrix<T, C, S> const &array, linalg::Matrix<T, C, S> &ret)
{
  assert(ret.size() == array.size());
  assert(ret.shape() == array.shape());

  details::SoftmaxImplementation(array, ret);
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Softmax(linalg::Matrix<T, C, S> const &array)
{
  linalg::Matrix<T, C, S> ret{array.size()};
  ret.Reshape(array.shape());
  Softmax(array, ret);
  return ret;
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
void Maximum(ShapeLessArray<T, C> const &array1, ShapeLessArray<T, C> const &array2,
             ShapeLessArray<T, C> &ret)
{
  details::MaximumImplementation(array1, array2, ret);
}
template <typename T, typename C>
ShapeLessArray<T, C> Maximum(ShapeLessArray<T, C> const &array1, ShapeLessArray<T, C> const &array2)
{
  ShapeLessArray<T, C> ret(array1.size());
  Maximum(array1, array2, ret);
  return ret;
}

template <typename T, typename C, typename S>
void Maximum(linalg::Matrix<T, C, S> const &array1, linalg::Matrix<T, C, S> const &array2,
             linalg::Matrix<T, C, S> &ret)
{
  details::MaximumImplementation(array1, array2, ret);
}
template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Maximum(linalg::Matrix<T, C, S> const &array1,
                                linalg::Matrix<T, C, S> const &array2)
{
  std::vector<std::size_t> return_shape(array1.shape());
  linalg::Matrix<T, C, S>  ret(return_shape);
  Maximum(array1, array2, ret);
  return ret;
}

template <typename T, typename C, typename S>
linalg::Matrix<T, C, S> Maximum(linalg::Matrix<T, C, S> const &array1, T const &scalar)
{
  std::vector<std::size_t> return_shape(array1.shape());
  linalg::Matrix<T, C, S>  ret(return_shape);
  linalg::Matrix<T, C, S>  compare(return_shape);
  compare.Fill(scalar);
  Maximum(array1, compare, ret);
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
void Product(ShapeLessArray<T, C> const &obj1, T &ret)
{
  ret = obj1.data().in_parallel().Reduce(
      memory::TrivialRange(0, obj1.size()),
      [](typename ShapeLessArray<T, C>::vector_register_type const &a,
         typename ShapeLessArray<T, C>::vector_register_type const &b) ->
      typename ShapeLessArray<T, C>::vector_register_type { return a * b; });
}
template <typename T, typename C>
T Product(ShapeLessArray<T, C> const &obj1)
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
void Sum(ShapeLessArray<T, C> const &obj1, T &ret)
{
  ret = obj1.data().in_parallel().Reduce(
      memory::TrivialRange(0, obj1.size()),
      [](typename ShapeLessArray<T, C>::vector_register_type const &a,
         typename ShapeLessArray<T, C>::vector_register_type const &b) ->
      typename ShapeLessArray<T, C>::vector_register_type { return a + b; });
}
template <typename T, typename C>
T Sum(ShapeLessArray<T, C> const &obj1)
{
  T ret;
  Sum(obj1, ret);
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
void Mean(ShapeLessArray<T, C> const &obj1, T &ret)
{

  Sum(obj1, ret);
  Divide(ret, T(obj1.size()), ret);
}
template <typename T, typename C>
T Mean(ShapeLessArray<T, C> const &obj1)
{
  T ret;
  Mean(obj1, ret);
  return ret;
}

/**
 * Distance between max and min values in an array
 */
template <typename ArrayType>
void PeakToPeak(ArrayType arr)
{
  return Max(arr) - Min(arr);
}

}  // namespace math
}  // namespace fetch
