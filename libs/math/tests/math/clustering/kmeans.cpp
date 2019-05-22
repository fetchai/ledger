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

#include <gtest/gtest.h>
#include <iostream>

#include "math/clustering/k_means.hpp"
#include "math/combinatorics.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <math/tensor.hpp>
#include <string>
#include <vector>

using namespace fetch::math;

using DataType     = std::int64_t;
using ArrayType    = Tensor<DataType>;

using ClusteringType = fetch::math::clustering::ClusteringType;

TEST(clustering_test, kmeans_test_2d_4k)
{
  ArrayType    A({100, 2});
  ArrayType    ret({100, 1});
  SizeType K = 4;

  for (SizeType i = 0; i < 25; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, -static_cast<DataType>(i) - 50);
    A.Set(SizeType{i}, SizeType{1}, -static_cast<DataType>(i) - 50);
  }
  for (SizeType i = 25; i < 50; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, -static_cast<DataType>(i) - 50);
    A.Set(SizeType{i}, SizeType{1}, static_cast<DataType>(i) + 50);
  }
  for (SizeType i = 50; i < 75; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, static_cast<DataType>(i) + 50);
    A.Set(SizeType{i}, SizeType{1}, -static_cast<DataType>(i) - 50);
  }
  for (SizeType i = 75; i < 100; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, static_cast<DataType>(i) + 50);
    A.Set(SizeType{i}, SizeType{1}, static_cast<DataType>(i) + 50);
  }

  SizeType   random_seed = 123456;
  ClusteringType clusters    = fetch::math::clustering::KMeans(A, random_seed, K);

  SizeType group_0 = static_cast<SizeType>(clusters[0]);
  for (SizeType j = 0; j < 25; ++j)
  {
    ASSERT_EQ(group_0, static_cast<SizeType>(clusters[j]));
  }
  SizeType group_1 = static_cast<SizeType>(clusters[25]);
  for (SizeType j = 25; j < 50; ++j)
  {
    ASSERT_EQ(group_1, static_cast<SizeType>(clusters[j]));
  }
  SizeType group_2 = static_cast<SizeType>(clusters[50]);
  for (SizeType j = 50; j < 75; ++j)
  {
    ASSERT_EQ(group_2, static_cast<SizeType>(clusters[j]));
  }
  SizeType group_3 = static_cast<SizeType>(clusters[75]);
  for (SizeType j = 75; j < 100; ++j)
  {
    ASSERT_EQ(group_3, static_cast<SizeType>(clusters[j]));
  }
}

TEST(clustering_test, kmeans_test_previous_assignment)
{
  SizeType n_points = 50;

  SizeType   K = 2;
  ArrayType      A({n_points, 2});
  ClusteringType prev_k{n_points};
  ArrayType      ret(std::vector<SizeType>({n_points, 1}));

  for (SizeType i = 0; i < 25; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, static_cast<DataType>(-i - 50));
    A.Set(SizeType{i}, SizeType{1}, static_cast<DataType>(-i - 50));
  }
  for (SizeType i = 25; i < 50; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, static_cast<DataType>(i + 50));
    A.Set(SizeType{i}, SizeType{1}, static_cast<DataType>(i + 50));
  }

  for (SizeType i = 0; i < 5; ++i)
  {
    prev_k.Set(i, 0);
  }
  for (SizeType i = 5; i < 25; ++i)
  {
    prev_k.Set(i, -1);
  }
  for (SizeType i = 25; i < 30; ++i)
  {
    prev_k.Set(i, 1);
  }
  for (SizeType i = 30; i < 50; ++i)
  {
    prev_k.Set(i, -1);
  }

  SizeType   random_seed = 123456;
  ClusteringType clusters    = fetch::math::clustering::KMeans(A, random_seed, K, prev_k);

  SizeType group_0 = static_cast<SizeType>(clusters[0]);
  for (SizeType j = 0; j < 25; ++j)
  {
    ASSERT_EQ(group_0, static_cast<SizeType>(clusters[j]));
  }
  SizeType group_1 = static_cast<SizeType>(clusters[25]);
  for (SizeType j = 25; j < 50; ++j)
  {
    ASSERT_EQ(group_1, static_cast<SizeType>(clusters[j]));
  }
}

TEST(clustering_test, kmeans_simple_previous_assignment_no_K)
{
  SizeType n_points = 50;

  ArrayType      A(std::vector<SizeType>({n_points, 2}));
  ClusteringType prev_k{n_points};
  ArrayType      ret(std::vector<SizeType>({n_points, 1}));

  // initialise the data to be first half negative, second half positive
  for (SizeType i = 0; i < 25; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, static_cast<DataType>(-i - 50));
    A.Set(SizeType{i}, SizeType{1}, static_cast<DataType>(-i - 50));
  }
  for (SizeType i = 25; i < 50; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, static_cast<DataType>(i + 50));
    A.Set(SizeType{i}, SizeType{1}, static_cast<DataType>(i + 50));
  }

  // initialise the previous assignments to be 5 data points assigned, 20 unassigned in each
  for (SizeType i = 0; i < 5; ++i)
  {
    prev_k.Set(i, 0);
  }
  for (SizeType i = 5; i < 25; ++i)
  {
    prev_k.Set(i, -1);
  }
  for (SizeType i = 25; i < 30; ++i)
  {
    prev_k.Set(i, 1);
  }
  for (SizeType i = 30; i < 50; ++i)
  {
    prev_k.Set(i, -1);
  }

  SizeType                            random_seed = 123456;
  fetch::math::clustering::KInferenceMode k_inference_mode =
      fetch::math::clustering::KInferenceMode::NClusters;
  ClusteringType clusters =
      fetch::math::clustering::KMeans<ArrayType>(A, random_seed, prev_k, k_inference_mode);

  SizeType group_0 = 0;
  for (SizeType j = 0; j < 25; ++j)
  {
    ASSERT_EQ(group_0, static_cast<SizeType>(clusters[j]));
  }
  SizeType group_1 = 1;
  for (SizeType j = 25; j < 50; ++j)
  {
    ASSERT_EQ(group_1, static_cast<SizeType>(clusters[j]));
  }
}

TEST(clustering_test, kmeans_remap_previous_assignment_no_K)
{
  SizeType n_points = 100;

  int no_group = -1;
  int group_0  = 17;
  int group_1  = 1;
  int group_2  = 156;
  int group_3  = 23;

  ArrayType      A(std::vector<SizeType>({n_points, 2}));
  ClusteringType prev_k{n_points};
  ArrayType      ret(std::vector<SizeType>({n_points, 1}));

  // assign data to 4 quadrants
  for (SizeType i = 0; i < 25; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, -static_cast<DataType>(i) - 50);
    A.Set(SizeType{i}, SizeType{1}, -static_cast<DataType>(i) - 50);
  }
  for (SizeType i = 25; i < 50; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, -static_cast<DataType>(i) - 50);
    A.Set(SizeType{i}, SizeType{1}, static_cast<DataType>(i) + 50);
  }
  for (SizeType i = 50; i < 75; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, static_cast<DataType>(i) + 50);
    A.Set(SizeType{i}, SizeType{1}, -static_cast<DataType>(i) - 50);
  }
  for (SizeType i = 75; i < 100; ++i)
  {
    A.Set(SizeType{i}, SizeType{0}, static_cast<DataType>(i) + 50);
    A.Set(SizeType{i}, SizeType{1}, static_cast<DataType>(i) + 50);
  }

  // assign previous cluster groups with some gaps
  for (SizeType i = 0; i < 5; ++i)
  {
    prev_k.Set(i, group_0);
  }
  for (SizeType i = 5; i < 25; ++i)
  {
    prev_k.Set(i, no_group);
  }
  for (SizeType i = 25; i < 30; ++i)
  {
    prev_k.Set(i, group_1);
  }
  for (SizeType i = 30; i < 50; ++i)
  {
    prev_k.Set(i, no_group);
  }

  for (SizeType i = 50; i < 55; ++i)
  {
    prev_k.Set(i, group_2);
  }
  for (SizeType i = 55; i < 75; ++i)
  {
    prev_k.Set(i, no_group);
  }
  for (SizeType i = 75; i < 80; ++i)
  {
    prev_k.Set(i, group_3);
  }
  for (SizeType i = 80; i < 100; ++i)
  {
    prev_k.Set(i, no_group);
  }

  SizeType                            random_seed = 123456;
  fetch::math::clustering::KInferenceMode k_inference_mode =
      fetch::math::clustering::KInferenceMode::NClusters;
  ClusteringType clusters =
      fetch::math::clustering::KMeans(A, random_seed, prev_k, k_inference_mode);

  for (SizeType j = 0; j < 25; ++j)
  {
    ASSERT_EQ(group_0, static_cast<int>(clusters[j]));
  }
  for (SizeType j = 25; j < 50; ++j)
  {
    ASSERT_EQ(group_1, static_cast<int>(clusters[j]));
  }
  for (SizeType j = 50; j < 75; ++j)
  {
    ASSERT_EQ(group_2, static_cast<int>(clusters[j]));
  }
  for (SizeType j = 75; j < 100; ++j)
  {
    ASSERT_EQ(group_3, static_cast<int>(clusters[j]));
  }
}
