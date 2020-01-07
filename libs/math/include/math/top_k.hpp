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

namespace fetch {
namespace math {

/**
 * makes array of k-highest values and array of their indices
 * for input array of shape [x,y,z] and value k, return arrays would be of shape [x,y,k]
 * Implementation based on tf.math.top_k
 * @tparam ArrayDataType
 * @tparam ArrayIndicesType
 * @param ret_data array of k-highest values
 * @param ret_indices array of indices of k-highest values
 * @param data input
 * @param k number of k-highest values
 * @param sorted TRUE=descending order, FALSE=ascending order
 */
template <typename ArrayDataType, typename ArrayIndicesType>
void TopK(ArrayDataType &ret_data, ArrayIndicesType &ret_indices, ArrayDataType const &data,
          typename ArrayDataType::SizeType k, fetch::math::SizeType axis, bool sorted = true)
{
  using DataType  = typename ArrayDataType::Type;
  using IndexType = typename ArrayIndicesType::Type;

  assert(axis < data.shape().size());
  assert(k <= data.shape().at(axis));

  SizeType axis_size = data.shape().at(axis);

  // Create iterators
  auto data_it        = data.Slice().cbegin();
  auto ret_data_it    = ret_data.Slice().begin();
  auto ret_indices_it = ret_indices.Slice().begin();

  // Set itaration order
  data_it.MoveAxisToFront(axis);
  ret_data_it.MoveAxisToFront(axis);
  ret_indices_it.MoveAxisToFront(axis);

  // Prepare vector for sorting values
  std::vector<std::pair<SizeType, DataType>> vec;
  vec.resize(axis_size);

  SizeType i           = 0;
  SizeType k_minus_one = k - 1;
  while (data_it.is_valid())
  {
    // Copy values to vector
    for (SizeType index{0}; index < axis_size; index++)
    {
      vec.at(index).first  = index;
      vec.at(index).second = *data_it;
      ++data_it;
    }

    // sort the top k values
    std::sort(vec.begin(), vec.end(),
              [](std::pair<IndexType, DataType> const &a, std::pair<IndexType, DataType> const &b) {
                return a.second > b.second;
              });

    // Copy sorted values to return array
    for (SizeType index{0}; index < k; index++)
    {
      // Normal order
      if (sorted)
      {
        i = index;
      }
      // Reverse order
      else
      {
        i = k_minus_one - index;
      }

      (*ret_data_it)    = vec.at(i).second;
      (*ret_indices_it) = vec.at(i).first;

      ++ret_data_it;
      ++ret_indices_it;
    }
  }
}

/**
 * makes pair of k-highest values and their indices
 * for input array of shape [x,y,z] and k returns pair of arrays {[x,y,k],[x,y,k]}
 * Implementation based on tf.math.top_k
 * @tparam ArrayDataType
 * @tparam ArrayIndicesType
 * @param data input
 * @param k number of k-highest values
 * @param sorted TRUE=descending order, FALSE=ascending order
 * @return std::pair<ArrayDataType,ArrayIndicesType>
 */
template <typename ArrayDataType, typename ArrayIndicesType>
std::pair<ArrayDataType, ArrayIndicesType> TopK(ArrayDataType const &            data,
                                                typename ArrayDataType::SizeType k,
                                                fetch::math::SizeType axis, bool sorted = true)
{
  assert(axis < data.shape().size());
  assert(k <= data.shape().at(axis));

  std::vector<SizeType> ret_shape = data.shape();
  ret_shape.at(axis)              = k;

  ArrayDataType    ret_data(ret_shape);
  ArrayIndicesType ret_indices(ret_shape);

  TopK(ret_data, ret_indices, data, k, axis, sorted);
  return std::pair<ArrayDataType, ArrayIndicesType>(ret_data, ret_indices);
}

}  // namespace math
}  // namespace fetch
