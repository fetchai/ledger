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
#include <algorithm>
#include <vector>

namespace fetch {
namespace math {

namespace details {

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;

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

}  // namespace details
}  // namespace math
}  // namespace fetch