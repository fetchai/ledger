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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include "math/free_functions/clustering_algorithms/k_means.hpp"
#include <math/linalg/matrix.hpp>

using namespace fetch::math::linalg;

using data_type      = double;
using container_type = fetch::memory::SharedArray<data_type>;
using matrix_type    = Matrix<data_type, container_type>;

TEST(clustering_test, kmeans_test)
{

  matrix_type A{100, 2};
  matrix_type ret{100, 1};
  std::size_t K = 2;

  for (std::size_t i = 0; i < 50; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(-i - 50.));
    A.Set(i, 1, static_cast<data_type>(-i - 50.));
  }
  for (std::size_t i = 50; i < 100; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(i));
    A.Set(i, 1, static_cast<data_type>(i));
  }

  auto now    = std::chrono::system_clock::now();
  auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  auto r_seed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now_ms.time_since_epoch()).count();
  std::size_t random_seed = static_cast<std::size_t>(r_seed);
  matrix_type clusters    = fetch::math::clustering::KMeans(A, K, random_seed);

  std::size_t group_0 = static_cast<std::size_t>(clusters[0]);
  for (std::size_t j = 0; j < clusters.size() / 2; ++j)
  {
    ASSERT_TRUE(group_0 == static_cast<std::size_t>(clusters[j]));
  }
  std::size_t group_1 = static_cast<std::size_t>(clusters[clusters.size() / 2]);
  for (std::size_t j = clusters.size() / 2; j < clusters.size(); ++j)
  {
    ASSERT_TRUE(group_1 == static_cast<std::size_t>(clusters[j]));
  }
}
