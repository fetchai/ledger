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

#include "math/distance/cosine.hpp"
#include <utility>

namespace fetch {
namespace math {
namespace clustering {
namespace details {

template <typename ArrayType>
std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> GetKNNImplementation(
    ArrayType array, ArrayType one_vector, typename ArrayType::SizeType k)
{
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;

  assert(one_vector.shape().size() == 2);
  assert(array.shape().size() == 2);
  assert(array.shape().at(1) == one_vector.shape().at(1));
  assert(one_vector.shape().at(0) == 1);

  std::vector<std::pair<SizeType, DataType>> similarities;
  similarities.reserve(array.shape().at(0));
  for (SizeType i(0); i < array.shape().at(0); ++i)
  {
    typename ArrayType::Type d =
        fetch::math::distance::Cosine(one_vector, array.Slice(i).Unsqueeze());
    similarities.emplace_back(i, d);
  }

  std::nth_element(similarities.begin(), similarities.begin() + unsigned(k), similarities.end(),
                   [](std::pair<SizeType, DataType> const &a,
                      std::pair<SizeType, DataType> const &b) { return a.second > b.second; });

  // fill the return container with the top K
  std::vector<std::pair<SizeType, DataType>> ret;
  for (SizeType i(0); i < k; ++i)
  {
    ret.emplace_back(std::make_pair(similarities.at(i).first, similarities.at(i).second));
  }

  std::sort(ret.begin(), ret.end(),
            [](std::pair<SizeType, DataType> const &a, std::pair<SizeType, DataType> const &b) {
              return a.second > b.second;
            });

  return ret;
}

}  // namespace details

/**
 * Interface to get K nearest neighbours method comparing array with input vector
 * @tparam ArrayType  template for type of array
 * @param array   array of shape # data points X # feature dimensions
 * @param k  value of k - i.e. how many nearest data points to find
 */
template <typename ArrayType>
std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> KNN(
    ArrayType array, ArrayType one_vector, typename ArrayType::SizeType k)
{
  return details::GetKNNImplementation(array, one_vector, k);
}

/**
 * Interface to get K nearest neighbours method comparing array with input vector
 * @tparam ArrayType  template for type of array
 * @param array   array of shape # data points X # feature dimensions
 * @param k  value of k - i.e. how many nearest data points to find
 */
template <typename ArrayType>
std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> KNN(
    ArrayType array, typename ArrayType::SizeType idx, typename ArrayType::SizeType k)
{
  ArrayType one_vector = array.slice(idx);
  return details::GetKNNImplementation(array, one_vector, k);
}

}  // namespace clustering
}  // namespace math
}  // namespace fetch