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

TEST(clustering_test, kmeans_test_2d_4k)
{

  matrix_type A{100, 2};
  matrix_type ret{100, 1};
  std::size_t K = 2;

  for (std::size_t i = 0; i < 25; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(-i - 50));
    A.Set(i, 1, static_cast<data_type>(-i - 50));
  }
  for (std::size_t i = 25; i < 50; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(-i - 50));
    A.Set(i, 1, static_cast<data_type>(i + 50));
  }
  for (std::size_t i = 50; i < 75; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(i + 50));
    A.Set(i, 1, static_cast<data_type>(-i - 50));
  }
  for (std::size_t i = 75; i < 100; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(i + 50));
    A.Set(i, 1, static_cast<data_type>(i + 50));
  }

  std::size_t random_seed = 123456;
  matrix_type clusters    = fetch::math::clustering::KMeans(A, K, random_seed);

  std::size_t group_0 = static_cast<std::size_t>(clusters[0]);
  for (std::size_t j = 0; j < 25; ++j)
  {
    ASSERT_TRUE(group_0 == static_cast<std::size_t>(clusters[j]));
  }
  std::size_t group_1 = static_cast<std::size_t>(clusters[25]);
  for (std::size_t j = 25; j < 50; ++j)
  {
    ASSERT_TRUE(group_1 == static_cast<std::size_t>(clusters[j]));
  }
  std::size_t group_2 = static_cast<std::size_t>(clusters[50]);
  for (std::size_t j = 50; j < 75; ++j)
  {
    ASSERT_TRUE(group_2 == static_cast<std::size_t>(clusters[j]));
  }
  std::size_t group_3 = static_cast<std::size_t>(clusters[75]);
  for (std::size_t j = 75; j < 100; ++j)
  {
    ASSERT_TRUE(group_3 == static_cast<std::size_t>(clusters[j]));
  }
}

TEST(clustering_test, kmeans_test_previous_assignment)
{
  std::size_t n_points = 50;

  std::size_t K = 2;
  matrix_type A{n_points, 2};
  matrix_type prev_k{n_points, 1};
  matrix_type ret{n_points, 1};

  for (std::size_t i = 0; i < 25; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(-i - 50));
    A.Set(i, 1, static_cast<data_type>(-i - 50));
  }
  for (std::size_t i = 25; i < 50; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(i + 50));
    A.Set(i, 1, static_cast<data_type>(i + 50));
  }

  for (std::size_t i = 0; i < 5; ++i)
  {
    prev_k.Set(i, 0, 0);
  }
  for (std::size_t i = 5; i < 25; ++i)
  {
    prev_k.Set(i, 0, -1);
  }
  for (std::size_t i = 25; i < 30; ++i)
  {
    prev_k.Set(i, 0, 1);
  }
  for (std::size_t i = 30; i < 50; ++i)
  {
    prev_k.Set(i, 0, -1);
  }

  std::size_t random_seed = 123456;
  matrix_type clusters    = fetch::math::clustering::KMeans(A, K, random_seed, prev_k);

  std::size_t group_0 = static_cast<std::size_t>(clusters[0]);
  for (std::size_t j = 0; j < 25; ++j)
  {
    ASSERT_TRUE(group_0 == static_cast<std::size_t>(clusters[j]));
  }
  std::size_t group_1 = static_cast<std::size_t>(clusters[25]);
  for (std::size_t j = 25; j < 50; ++j)
  {
    ASSERT_TRUE(group_1 == static_cast<std::size_t>(clusters[j]));
  }
}
