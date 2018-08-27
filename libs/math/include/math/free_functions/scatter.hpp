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

namespace fetch {
namespace math {
namespace free_functions {

/**
 * Copies the values of updates into the specified indices of the first dimension of data in this
 * object
 */
template <typename T, typename C>
struct Scatter
{
  void operator()(NDArray<T, C> &input_array, std::vector<T> &updates, std::vector<std::uint64_t> &indices) const
  {
    // sort indices and updates into ascending order

    std::vector<std::pair<std::uint64_t, T>> AB;

    //copy into pairs
    //Note that A values are put in "first" this is very important
    for(std::size_t i = 0; i<updates.size(); ++i)
    {
      AB.push_back(std::make_pair(indices[i], updates[i]));
    }

    std::sort(AB.begin(), AB.end());

    //Place back into arrays
    for( size_t i = 0; i<updates.size(); ++i)
    {
      updates[i] = AB[i].second;  //<- This is actually optional
      indices[i] = AB[i].first;
    }


    assert(indices.back() <= input_array.shape()[0]);

    // set up an iterator
    NDArrayIterator<T, typename NDArray<T, C>::container_type> arr_iterator{input_array};

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
};



}  // namespace free_functions
}  // namespace math
}  // namespace fetch
