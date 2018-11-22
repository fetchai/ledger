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
#include "math/free_functions/standard_functions/combinatorics.hpp"
#include <math/linalg/matrix.hpp>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <chrono>

using namespace fetch::math::linalg;

using data_type      = double;
using container_type = fetch::memory::SharedArray<data_type>;
using matrix_type    = Matrix<data_type, container_type>;


// Helper function for kmeans_test_ndimensions
std::size_t add_cluster_to_matrix(int n_points_per_cluster, std::size_t n_dimensions, std::vector<int> dimension_signs,
    std::size_t row, matrix_type &A, int initial_val_magnitude){
  int val_magnitude = initial_val_magnitude;
  // Create specified number of points in each cluster
  for (std::size_t p=0; p < static_cast<std::size_t>(n_points_per_cluster); ++p){
    // Determine values along each dimension for the current point
    for (std::size_t k=0; k<static_cast<std::size_t>(n_dimensions); ++k)
    {
      // Assign value to current dimension (column in array A) of current point (row in array A), starting at input row
      int sign = dimension_signs[k];
      int val = val_magnitude * sign;
      A.Set(row, k, val);
    }
    row += 1;
    // Increment magnitude of values within cluster so not all points are identical in each cluster
    val_magnitude += 5;
  }
  return row;
}


TEST(clustering_test, kmeans_test_2d_4k)
{

  matrix_type A{100, 2};
  matrix_type ret{100, 1};
  std::size_t K = 4;

  for (std::size_t i = 0; i < 25; ++i)
  {
    A.Set(i, 0, -static_cast<data_type>(i) - 50);
    A.Set(i, 1, -static_cast<data_type>(i) - 50);
  }
  for (std::size_t i = 25; i < 50; ++i)
  {
    A.Set(i, 0, -static_cast<data_type>(i) - 50);
    A.Set(i, 1, static_cast<data_type>(i) + 50);
  }
  for (std::size_t i = 50; i < 75; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(i) + 50);
    A.Set(i, 1, -static_cast<data_type>(i) - 50);
  }
  for (std::size_t i = 75; i < 100; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(i) + 50);
    A.Set(i, 1, static_cast<data_type>(i) + 50);
  }

  std::size_t random_seed = 123456;
  matrix_type clusters    = fetch::math::clustering::KMeans(A, random_seed, K);

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


TEST(clustering_test, kmeans_test_ndimensions)
{

  std::size_t n_dimensions = 5;
  float base = 2.0;
  // Each dimension will be positive or negative for 2^n combinations
  float out = std::pow(base, static_cast<float>(n_dimensions));
  int n_clusters = static_cast<int>(out);
  int n_points_per_cluster = 5;
  int n_points = n_clusters * n_points_per_cluster;
  matrix_type A{static_cast<std::size_t>(n_points), static_cast<std::size_t>(n_dimensions)};

  // Trivial case: all dimensions are negative
  std::vector<int> dimension_signs (static_cast<std::size_t>(n_dimensions), -1);
  std::size_t row = 0;
  int initial_val_magnitude = 50;
  // Initialize first point in each cluster with magnitude 50 in each dimension
  std::size_t next_row = add_cluster_to_matrix(n_points_per_cluster, n_dimensions, dimension_signs, row, A,
      initial_val_magnitude);
  row = next_row;

  // r represents number of dimensions that are positive
  for (std::size_t r = 1; r <= n_dimensions; ++r){
    // Each row of array tells us which dimensions are positive
    matrix_type combinations_array = fetch::math::combinatorics::combinations(n_dimensions, r);

    // Create vector of size (1 x n_dimensions) that indicates whether dimension is positive or negative in current
    // cluster
    for (std::size_t i = 0; i < combinations_array.shape()[0]; ++i){
      std::vector<int> dimension_signs (static_cast<std::size_t>(n_dimensions), -1);
      for (std::size_t j = 0; j < combinations_array.shape()[1]; ++j){
        int positive_dimension = static_cast<int>(combinations_array.At(i,j)) - 1;
        dimension_signs[static_cast<std::size_t>(positive_dimension)] = 1;
      }

      std::size_t next_row = add_cluster_to_matrix(n_points_per_cluster, n_dimensions, dimension_signs, row, A,
                                                   initial_val_magnitude);
      row = next_row;
    }
  }

  std::size_t random_seed = 123456;
  matrix_type clusters = fetch::math::clustering::KMeans(A, random_seed, static_cast<std::size_t>(n_clusters));

  for (std::size_t c = 0; c < static_cast<std::size_t>(n_clusters); ++c){
    auto current_cluster = static_cast<std::size_t>(clusters[c]);
    for (std::size_t p = 0; p < static_cast<std::size_t>(n_points); ++p){
      ASSERT_TRUE(current_cluster == static_cast<std::size_t>(clusters[c]));
    }
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
  matrix_type clusters    = fetch::math::clustering::KMeans(A, random_seed, K, prev_k);

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

TEST(clustering_test, kmeans_test_previous_assignment_no_K_simple)
{
  std::size_t n_points = 50;

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

  std::size_t                             random_seed = 123456;
  fetch::math::clustering::KInferenceMode k_inference_mode =
      fetch::math::clustering::KInferenceMode::NClusters;
  matrix_type clusters =
      fetch::math::clustering::KMeans<matrix_type>(A, random_seed, prev_k, k_inference_mode);

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

TEST(clustering_test, kmeans_test_previous_assignment_no_K_complex)
{
  std::size_t n_points = 100;

  matrix_type A{n_points, 2};
  matrix_type prev_k{n_points, 1};
  matrix_type ret{n_points, 1};

  for (std::size_t i = 0; i < 25; ++i)
  {
    A.Set(i, 0, -static_cast<data_type>(i) - 50);
    A.Set(i, 1, -static_cast<data_type>(i) - 50);
  }
  for (std::size_t i = 25; i < 50; ++i)
  {
    A.Set(i, 0, -static_cast<data_type>(i) - 50);
    A.Set(i, 1, static_cast<data_type>(i) + 50);
  }
  for (std::size_t i = 50; i < 75; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(i) + 50);
    A.Set(i, 1, -static_cast<data_type>(i) - 50);
  }
  for (std::size_t i = 75; i < 100; ++i)
  {
    A.Set(i, 0, static_cast<data_type>(i) + 50);
    A.Set(i, 1, static_cast<data_type>(i) + 50);
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

  for (std::size_t i = 50; i < 75; ++i)
  {
    prev_k.Set(i, 0, -1);
  }
  for (std::size_t i = 75; i < 80; ++i)
  {
    prev_k.Set(i, 0, 1);
  }
  for (std::size_t i = 80; i < 100; ++i)
  {
    prev_k.Set(i, 0, -1);
  }

  std::size_t                             random_seed = 123456;
  fetch::math::clustering::KInferenceMode k_inference_mode =
      fetch::math::clustering::KInferenceMode::NClusters;
  matrix_type clusters = fetch::math::clustering::KMeans(A, random_seed, prev_k, k_inference_mode);

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
