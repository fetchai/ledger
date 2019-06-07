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

template <typename ArrayType,
          typename ArrayType::Type (*Distance)(ArrayType const &, ArrayType const &)>
std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> KNNImplementation(
    ArrayType array, ArrayType vec, typename ArrayType::SizeType k)
{
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;

  assert(vec.shape().size() == 2);
  assert(array.shape().size() == 2);

  // test vector must be either {1, N} or {N, 1}
  assert((vec.shape().at(0) == 1) || (vec.shape().at(1) == 1));

  // array must be either {M, N} (if vector shape is {1, N}) or {N, M}
  assert(((array.shape().at(1) == vec.shape().at(1)) && (vec.shape().at(0) == 1)) ||
         ((array.shape().at(0) == vec.shape().at(0)) && (vec.shape().at(1) == 1)));

  // vec <1, M>; array <N, M>
  //  assert(array.shape().at(1) == vec.shape().at(1));

  SizeType feature_axis;
  SizeType data_axis;
  if (vec.shape().at(0) == 1)
  {
    feature_axis = 1;
  }
  else
  {
    feature_axis = 0;
  }
  data_axis = 1 - feature_axis;

  std::vector<std::pair<SizeType, DataType>> similarities;
  std::vector<std::pair<SizeType, DataType>> ret;

  similarities.reserve(array.shape().at(data_axis));

  // compute distances
  for (SizeType i = 0; i < array.shape().at(data_axis); ++i)
  {
    typename ArrayType::Type d = Distance(vec, array.Slice(i, 1).Copy());
    similarities.emplace_back(i, d);
  }

  // partial sort first K values
  std::nth_element(similarities.begin(), similarities.begin() + unsigned(k), similarities.end(),
                   [](std::pair<SizeType, DataType> const &a,
                      std::pair<SizeType, DataType> const &b) { return a.second < b.second; });

  // fill the return container with the partial sort top k
  for (SizeType i(0); i < k; ++i)
  {
    ret.emplace_back(std::make_pair(similarities.at(i).first, similarities.at(i).second));
  }

  // sort the top k values
  std::sort(ret.begin(), ret.end(),
            [](std::pair<SizeType, DataType> const &a, std::pair<SizeType, DataType> const &b) {
              return a.second < b.second;
            });

  return ret;
}

}  // namespace details

/**
 */

/**
 * Interface to get K nearest neighbours method comparing array with input vector
 * Uses cosine distance function
 * @tparam ArrayType  template for type of array
 * @param array array of shape # data points X # feature dimensions
 * @param vec the test vector
 * @param k value of k - i.e. how many nearest data points to find
 * @return
 */
template <typename ArrayType>
std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> KNNCosine(
    ArrayType array, ArrayType vec, typename ArrayType::SizeType k)
{
  return details::KNNImplementation<ArrayType, fetch::math::distance::Cosine>(array, vec, k);
}

/**
 * Interface to get K nearest neighbours method comparing array with input vector
 * Uses cosine distance function
 * @tparam ArrayType  template for type of array
 * @param array   array of shape # data points X # feature dimensions
 * @param idx extract row of array to use as the test vector vector
 * @param k  value of k - i.e. how many nearest data points to find
 */
template <typename ArrayType>
std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> KNNCosine(
    ArrayType array, typename ArrayType::SizeType idx, typename ArrayType::SizeType k)
{
  ArrayType vec = array.slice(idx);
  return details::KNNImplementation<ArrayType, fetch::math::distance::Cosine>(array, vec, k);
}

/**
 * Interface to get K nearest neighbours method comparing array with input vector
 * Uses templated distance function Function
 * Can be called by: KNN<ArrayType,Function>(array,vec,k);
 * @tparam ArrayType  template for type of array
 * @tparam Distance   template for distance function in format ArrayType::Type(ArrayType const
 * &,ArrayType const &)
 * @param array   array of shape # data points X # feature dimensions
 * @param vec the test vector
 * @param k  value of k - i.e. how many nearest data points to find
 */
template <typename ArrayType,
          typename ArrayType::Type (*Distance)(ArrayType const &, ArrayType const &)>
std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> KNN(
    ArrayType array, ArrayType vec, typename ArrayType::SizeType k)
{
  return details::KNNImplementation<ArrayType, Distance>(array, vec, k);
}

/**
 * Interface to get K nearest neighbours method comparing array with input vector
 * Uses templated distance function Function
 * Can be called by: KNN<ArrayType,Function>(array,vec,k);
 * @tparam ArrayType  template for type of array
 * @tparam Distance   template for distance function in format ArrayType::Type(ArrayType const
 * &,ArrayType const &)
 * @param array   array of shape # data points X # feature dimensions
 * @param idx extract row of array to use as the test vector vector
 * @param k  value of k - i.e. how many nearest data points to find
 */
template <typename ArrayType,
          typename ArrayType::Type (*Distance)(ArrayType const &, ArrayType const &)>
std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> KNN(
    ArrayType array, typename ArrayType::SizeType idx, typename ArrayType::SizeType k)
{
  ArrayType vec = array.slice(idx);
  return details::KNNImplementation<ArrayType, Distance>(array, vec, k);
}

}  // namespace clustering
}  // namespace math
}  // namespace fetch
