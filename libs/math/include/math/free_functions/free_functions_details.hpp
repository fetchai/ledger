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

#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "math/ndarray_iterator.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <algorithm>
#include <vector>

namespace fetch {
namespace math {

namespace details {

// template <typename T, typename C>
// class ShapeLessArray;

template <typename ARRAY_TYPE>
void ScatterImplementation(ARRAY_TYPE &input_array, std::vector<typename ARRAY_TYPE::type> &updates,
                           std::vector<std::size_t> &indices)
{
  // sort indices and updates into ascending order

  std::vector<std::pair<std::size_t, typename ARRAY_TYPE::type>> AB;

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
    updates[i] = AB[i].second;  //<- This is actually optional
    indices[i] = AB[i].first;
  }

  //  assert(indices.back() <= input_array.shape()[0]);

  // set up an iterator
  NDArrayIterator<typename ARRAY_TYPE::type, typename ARRAY_TYPE::container_type> arr_iterator{
      input_array};

  // scatter
  std::size_t cur_idx, arr_count = 0;
  for (std::size_t count = 0; count < indices.size(); ++count)
  {
    cur_idx = indices[count];

    while (arr_count < cur_idx)
    {
      ++arr_iterator;
      ++arr_count;
    }

    *arr_iterator = updates[count];
  }
}

template <typename ARRAY_TYPE>
void GatherImplementation(ARRAY_TYPE &input_array, ARRAY_TYPE &updates,
                          std::vector<std::size_t> &indices)
{

  assert(input_array.size() == updates.size());
  input_array.LazyReshape(updates.shape());

  // sort indices
  std::sort(indices.begin(), indices.end());

  // check largest value in indices < shape()[0]
  assert(indices.back() <= updates.shape()[0]);

  // set up an iterator
  NDArrayIterator<typename ARRAY_TYPE::type, typename ARRAY_TYPE::container_type> arr_iterator{
      updates};
  NDArrayIterator<typename ARRAY_TYPE::type, typename ARRAY_TYPE::container_type> ret_iterator{
      input_array};

  std::size_t cur_idx, arr_count = 0;
  for (std::size_t count = 0; count < indices.size(); ++count)
  {
    cur_idx = indices[count];

    while (arr_count < cur_idx)
    {
      ++arr_iterator;
      ++arr_count;
    }

    *ret_iterator = *arr_iterator;
  }
}

template <typename ARRAY_TYPE>
void DynamicStitch(ARRAY_TYPE &input_array, std::vector<std::vector<std::size_t>> const &indices,
                   std::vector<ARRAY_TYPE> const &data)
{
  // identify the new size of this
  std::size_t new_size = 0;
  for (std::size_t l = 0; l < indices.size(); ++l)
  {
    new_size += indices[l].size();
  }

  input_array.LazyResize(new_size);

  // loop through all output data locations identifying the next data point to copy into it
  for (std::size_t i = 0; i < indices.size(); ++i)  // iterate through lists of indices
  {
    for (std::size_t k = 0; k < indices[i].size(); ++k)  // iterate through index within this list
    {
      assert(indices[i][k] <= input_array.size());
      input_array[indices[i][k]] = data[i][k];
    }
  }
}

template <typename ARRAY_TYPE>
void BooleanMaskImplementation(ARRAY_TYPE &input_array, ARRAY_TYPE const &mask)
{
  assert(input_array.size() == mask.size());

  std::size_t counter = 0;
  for (std::size_t i = 0; i < input_array.size(); ++i)
  {
    assert((mask[i] == 1) || (mask[i] == 0));
    // TODO(private issue 193): implement boolean only ndarray to avoid cast
    if (bool(mask[i]))
    {
      input_array[counter] = input_array[i];
      ++counter;
    }
  }

  input_array.LazyResize(counter);
}

/**
 * Basic implementation of Max for two variables
 * @tparam T data type
 * @param datum1 a single value
 * @param datum2 a second value
 * @param ret the maximum of the two values
 */
template <typename T>
inline void MaxImplementation(T const &datum1, T const &datum2, T &ret)
{
  ret = std::max(datum1, datum2);
}

/**
 * Implementaion of Max that returns the single largest value in the array
 * @tparam T
 * @param array
 * @param ret
 */
template <typename T, typename C>
inline void MaxImplementation(ShapeLessArray<T, C> const &         array,
                              typename ShapeLessArray<T, C>::type &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
}

/**
 * Implementation of Max that returns the single largest value in a range within the array
 * @tparam ARRAY_TYPE
 * @param array
 * @param r
 * @param ret
 */
template <typename T, typename C>
inline void MaxImplementation(ShapeLessArray<T, C> const &array, memory::Range r,
                              typename ShapeLessArray<T, C>::type &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  if (r.is_trivial())
  {
    ret = array.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return max(a, b); });
  }
  else
  {  // non-trivial range is not vectorised
    typename ShapeLessArray<T, C>::type ret =
        -std::numeric_limits<typename ShapeLessArray<T, C>::type>::max();
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
void MaxImplementation(NDArray<T, C> &array, std::size_t const &axis, NDArray<T, C> &ret)
{
  assert(axis < array.shape().size());

  NDArrayIterator<typename NDArray<T, C>::type, typename NDArray<T, C>::container_type>
      return_iterator{ret};

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
    NDArrayIterator<typename NDArray<T, C>::type, typename NDArray<T, C>::container_type>
        array_iterator(array, cur_step);

    // loops through the 1d array calculating the max val
    typename NDArray<T, C>::type cur_max =
        -std::numeric_limits<typename NDArray<T, C>::type>::max();
    typename NDArray<T, C>::type cur_val;
    while (array_iterator)
    {
      cur_val = *array_iterator;
      MaxImplementation(cur_max, cur_val, cur_max);
      ++array_iterator;
    }

    *return_iterator = cur_max;
    ++return_iterator;
  }
}

/**
 * Basic implementation of Min for two variables
 * @tparam T data type
 * @param datum1 a single value
 * @param datum2 a second value
 * @param ret the minimum of the two values
 */
template <typename T>
inline void MinImplementation(T const &datum1, T const &datum2, T &ret)
{
  ret = std::min(datum1, datum2);
}

/**
 * Implementation of Min that returns the single smallest value in the array
 * @tparam T
 * @param array
 * @param ret
 */
template <typename T, typename C>
inline void MinImplementation(ShapeLessArray<T, C> const &         array,
                              typename ShapeLessArray<T, C>::type &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  ret = array.data().in_parallel().Reduce(
      memory::TrivialRange(0, array.size()),
      [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });
}

/**
 * Implementation of Min that returns the single smallest value in a range within the array
 * @tparam ARRAY_TYPE
 * @param array
 * @param r
 * @param ret
 */
template <typename T, typename C>
inline void MinImplementation(ShapeLessArray<T, C> const &array, memory::Range r,
                              typename T::type &ret)
{
  using vector_register_type = typename ShapeLessArray<T, C>::vector_register_type;

  if (r.is_trivial())
  {
    ret = array.data().in_parallel().Reduce(
        r, [](vector_register_type const &a, vector_register_type const &b) { return min(a, b); });
  }
  else
  {  // non-trivial range is not vectorised
    typename T::type ret = std::numeric_limits<typename T::type>::max();
    for (auto i : array)
    {
      ret = std::min(ret, i);
    }
  }
}

template <typename T, typename C>
void MinImplementation(NDArray<T, C> &array, std::size_t const axis, NDArray<T, C> &ret)
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
      MinImplementation(cur_max, cur_val, cur_max);
      ++array_iterator;
    }

    *return_iterator = cur_max;
    ++return_iterator;
  }
}

template <typename T, typename C>
void SoftmaxImplementation(ShapeLessArray<T, C> const &array, ShapeLessArray<T, C> &ret)
{
  ret.LazyResize(array.size());

  // by subtracting the max we improve numerical stability, and the result will be identical
  ret.Subtract(array, array.Max());
  Exp(ret);
  ret.Divide(ret, ret.Sum());
}

}  // namespace details
}  // namespace math
}  // namespace fetch