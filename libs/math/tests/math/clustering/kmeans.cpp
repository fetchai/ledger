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
#include <iomanip>
#include <iostream>

#include "math/free_functions/clustering_algorithms/k_means.hpp"
#include "math/free_functions/combinatorics/combinatorics.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <math/shapeless_array.hpp>
#include <math/tensor.hpp>
#include <string>
#include <vector>

using namespace fetch::math;

using DataType   = std::int64_t;
using ArrayType = Tensor<DataType>;
using SizeType = Tensor<DataType>::SizeType;

using ClusteringType = fetch::math::clustering::ClusteringType;

// Helper function for kmeans_test_ndimensions
SizeType add_cluster_to_matrix(SizeType n_points_per_cluster, SizeType n_dimensions,
                                 std::vector<int> dimension_signs, SizeType row, ArrayType
                                 &A, int initial_val_magnitude)
{
 int val_magnitude = initial_val_magnitude;
 // Create specified number of points in each cluster
 for (SizeType p = 0; p < (n_points_per_cluster); ++p)
 {
   // Determine values along each dimension for the current point
   for (SizeType k = 0; k < n_dimensions; ++k)
   {
     // Assign value to current dimension (column in array A) of current point (row in array A),
     // starting at input row
     int sign = dimension_signs[k];
     int val  = val_magnitude * sign;
     A.Set(std::vector<SizeType>(k, row), val);
   }
   row += 1;

   // Increment magnitude of values within cluster so not all points are identical in each
   val_magnitude += 5;
 }
 return row;
}

TEST(clustering_test, kmeans_test_2d_4k)
{
 ArrayType  A({2, 100});
 ArrayType  ret({1, 100});
 SizeType K = 4;

 for (SizeType i = 0; i < 25; ++i)
 {
   A.Set(std::vector<SizeType>(0, i), -static_cast<DataType>(i) - 50);
   A.Set(std::vector<SizeType>(1, i), -static_cast<DataType>(i) - 50);
 }
 for (SizeType i = 25; i < 50; ++i)
 {
   A.Set(std::vector<SizeType>(0, i), -static_cast<DataType>(i) - 50);
   A.Set(std::vector<SizeType>(1, i), static_cast<DataType>(i) + 50);
 }
 for (SizeType i = 50; i < 75; ++i)
 {
   A.Set(std::vector<SizeType>(0, i), static_cast<DataType>(i) + 50);
   A.Set(std::vector<SizeType>(1, i), -static_cast<DataType>(i) - 50);
 }
 for (SizeType i = 75; i < 100; ++i)
 {
   A.Set(std::vector<SizeType>(0, i), static_cast<DataType>(i) + 50);
   A.Set(std::vector<SizeType>(1, i), static_cast<DataType>(i) + 50);
 }

 SizeType random_seed = 123456;
 ClusteringType clusters = fetch::math::clustering::KMeans(A, random_seed, K);

 SizeType group_0 = static_cast<SizeType>(clusters[0]);
 for (SizeType j = 0; j < 25; ++j)
 {
   ASSERT_TRUE(group_0 == static_cast<SizeType>(clusters[j]));
 }
 SizeType group_1 = static_cast<SizeType>(clusters[25]);
 for (SizeType j = 25; j < 50; ++j)
 {
   ASSERT_TRUE(group_1 == static_cast<SizeType>(clusters[j]));
 }
 SizeType group_2 = static_cast<SizeType>(clusters[50]);
 for (SizeType j = 50; j < 75; ++j)
 {
   ASSERT_TRUE(group_2 == static_cast<SizeType>(clusters[j]));
 }
 SizeType group_3 = static_cast<SizeType>(clusters[75]);
 for (SizeType j = 75; j < 100; ++j)
 {
   ASSERT_TRUE(group_3 == static_cast<SizeType>(clusters[j]));
 }
}

TEST(clustering_test, kmeans_test_ndimensions)
{

 SizeType n_dimensions = 5;
 float base = 2.0;

 // Each dimension will be positive or negative for 2^n combinations
 float out = std::pow(base, static_cast<float>(n_dimensions));
 SizeType n_clusters = static_cast<SizeType>(out);
 SizeType n_points_per_cluster = 5;
 SizeType n_points = n_clusters * n_points_per_cluster;
 ArrayType A({n_dimensions, n_points});

 // Trivial case: all dimensions are negative
 std::vector<int> dimension_signs(n_dimensions, -1);
 SizeType row = 0;
 int initial_val_magnitude = 50;
 // Initialize first point in each cluster with magnitude 50 in each dimension
 SizeType next_row = add_cluster_to_matrix(n_points_per_cluster, n_dimensions, dimension_signs, row, A, initial_val_magnitude);
 row = next_row;

 // r represents number of dimensions that are positive
 for (SizeType r = 1; r <= n_dimensions; ++r)
 {
   // Each row of array tells us which dimensions are positive
   ArrayType combinations_array = fetch::math::combinatorics::combinations<ArrayType>(n_dimensions, r);

   // Create vector of size (1 x n_dimensions) that indicates whether dimension is positive or
   // negative in current cluster
   for (SizeType i = 0; i < combinations_array.shape()[0]; ++i)
   {
     std::vector<int> dimension_signs(static_cast<SizeType>(n_dimensions), -1);
     for (SizeType j = 0; j < combinations_array.shape()[1]; ++j)
     {
       int positive_dimension = static_cast<int>(combinations_array.Get({j, i})) - 1;
       dimension_signs[static_cast<SizeType>(positive_dimension)] = 1;
     }

     SizeType next_row = add_cluster_to_matrix(n_points_per_cluster, n_dimensions, dimension_signs, row, A, initial_val_magnitude);
     row = next_row;
   }
 }

 SizeType    random_seed = 123456;
 ClusteringType clusters = fetch::math::clustering::KMeans(A, random_seed, static_cast<SizeType>(n_clusters));

 for (SizeType c = 0; c < static_cast<SizeType>(n_clusters); ++c)
 {
   auto current_cluster = static_cast<SizeType>(clusters[c]);
   for (SizeType p = 0; p < static_cast<SizeType>(n_points); ++p)
   {
     ASSERT_TRUE(current_cluster == static_cast<SizeType>(clusters[c]));
   }
 }
}

TEST(clustering_test, kmeans_test_previous_assignment)
{
 SizeType n_points = 50;

 SizeType    K = 2;
 ArrayType     A({2, n_points});
 ClusteringType prev_k{n_points};
 ArrayType     ret(std::vector<SizeType>({1, n_points}));

 for (SizeType i = 0; i < 25; ++i)
 {
   A.Set({0, i}, static_cast<DataType>(-i - 50));
   A.Set({1, i}, static_cast<DataType>(-i - 50));
 }
 for (SizeType i = 25; i < 50; ++i)
 {
   A.Set({0, i}, static_cast<DataType>(i + 50));
   A.Set({1, i}, static_cast<DataType>(i + 50));
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

 SizeType    random_seed = 123456;
 ClusteringType clusters    = fetch::math::clustering::KMeans(A, random_seed, K, prev_k);

 SizeType group_0 = static_cast<SizeType>(clusters[0]);
 for (SizeType j = 0; j < 25; ++j)
 {
   ASSERT_TRUE(group_0 == static_cast<SizeType>(clusters[j]));
 }
 SizeType group_1 = static_cast<SizeType>(clusters[25]);
 for (SizeType j = 25; j < 50; ++j)
 {
   ASSERT_TRUE(group_1 == static_cast<SizeType>(clusters[j]));
 }
}

TEST(clustering_test, kmeans_simple_previous_assignment_no_K)
{
 SizeType n_points = 50;

 ArrayType     A(std::vector<SizeType>({2, n_points}));
 ClusteringType prev_k{n_points};
 ArrayType     ret(std::vector<SizeType>({1, n_points}));

 // initialise the data to be first half negative, second half positive
 for (SizeType i = 0; i < 25; ++i)
 {
   A.Set({0, i}, static_cast<DataType>(-i - 50));
   A.Set({1, i}, static_cast<DataType>(-i - 50));
 }
 for (SizeType i = 25; i < 50; ++i)
 {
   A.Set({0, i}, static_cast<DataType>(i + 50));
   A.Set({1, i}, static_cast<DataType>(i + 50));
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

 SizeType                             random_seed = 123456;
 fetch::math::clustering::KInferenceMode k_inference_mode =
     fetch::math::clustering::KInferenceMode::NClusters;
 ClusteringType clusters =
     fetch::math::clustering::KMeans<ArrayType>(A, random_seed, prev_k, k_inference_mode);

 SizeType group_0 = 0;
 for (SizeType j = 0; j < 25; ++j)
 {
   ASSERT_TRUE(group_0 == static_cast<SizeType>(clusters[j]));
 }
 SizeType group_1 = 1;
 for (SizeType j = 25; j < 50; ++j)
 {
   ASSERT_TRUE(group_1 == static_cast<SizeType>(clusters[j]));
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

 ArrayType     A(std::vector<SizeType>({2, n_points}));
 ClusteringType prev_k{n_points};
 ArrayType     ret(std::vector<SizeType>({1, n_points}));

 // assign data to 4 quadrants
 for (SizeType i = 0; i < 25; ++i)
 {
   A.Set({0, i}, -static_cast<DataType>(i) - 50);
   A.Set({1, i}, -static_cast<DataType>(i) - 50);
 }
 for (SizeType i = 25; i < 50; ++i)
 {
   A.Set({0, i}, -static_cast<DataType>(i) - 50);
   A.Set({1, i}, static_cast<DataType>(i) + 50);
 }
 for (SizeType i = 50; i < 75; ++i)
 {
   A.Set({0, i}, static_cast<DataType>(i) + 50);
   A.Set({1, i}, -static_cast<DataType>(i) - 50);
 }
 for (SizeType i = 75; i < 100; ++i)
 {
   A.Set({0, i}, static_cast<DataType>(i) + 50);
   A.Set({1, i}, static_cast<DataType>(i) + 50);
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

 SizeType random_seed = 123456;
 fetch::math::clustering::KInferenceMode k_inference_mode = fetch::math::clustering::KInferenceMode::NClusters;
 ClusteringType clusters = fetch::math::clustering::KMeans(A, random_seed, prev_k, k_inference_mode);

 for (SizeType j = 0; j < 25; ++j)
 {
   ASSERT_TRUE(group_0 == static_cast<int>(clusters[j]));
 }
 for (SizeType j = 25; j < 50; ++j)
 {
   ASSERT_TRUE(group_1 == static_cast<int>(clusters[j]));
 }
 for (SizeType j = 50; j < 75; ++j)
 {
   ASSERT_TRUE(group_2 == static_cast<int>(clusters[j]));
 }
 for (SizeType j = 75; j < 100; ++j)
 {
   ASSERT_TRUE(group_3 == static_cast<int>(clusters[j]));
 }
}
