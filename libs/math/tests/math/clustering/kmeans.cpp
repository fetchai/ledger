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
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>

using namespace fetch::math::linalg;

using data_type      = double;
using container_type = fetch::memory::SharedArray<data_type>;
using matrix_type    = Matrix<data_type, container_type>;

int factorial(int n);
matrix_type combinations(int n, int r);

// Helper function
int factorial(int n)
{
  if (n == 0){
    return 1;
  }
  int i;
  for(i = n-1; i > 1; i--)
    n *= i;

  return n;
}

// Helper function
matrix_type combinations(int n, int r)
{
  if (r == 0){
    matrix_type output_array{};
    return output_array;
  }

  int n_combinations = factorial(n)/factorial(r)/factorial(n-r);
  std::size_t current_dim = 0;
  std::size_t current_row = 0;

  std::vector<bool> v(static_cast<unsigned long>(n));
  std::fill(v.end() - r, v.end(), true);

  matrix_type output_array{static_cast<std::size_t>(n_combinations), static_cast<std::size_t>(r)};
  do {
    for (int i = 0; i < n; ++i) {
      if (v[static_cast<unsigned long>(i)]) {
          int dim = (i+1);
          output_array.Set(current_row, current_dim, static_cast<data_type>(dim));
          if (current_dim == static_cast<std::size_t>(r)-1){
            current_row += 1;
          }
          current_dim = current_dim + 1;
          current_dim = current_dim % static_cast<std::size_t>(r);

        }
      }
    }while (std::next_permutation(v.begin(), v.end()));
    for (std::size_t k = 0; k < output_array.shape()[0]; ++k) {
        for (std::size_t j = 0;  j< output_array.shape()[1]; ++j)
        {
          std::cout << output_array.At(k,j) << "\t\t";
        }
        std::cout << std::endl;
      }
  return output_array;
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


TEST(clustering_test, kmeans_test_4dimensions)
{

  int n_dimensions = 4;
  float base = 2.0;
  float out = std::pow(base, static_cast<float>(n_dimensions));  // Each dimension will be positive or negative for 2^n combinations
  int n_clusters = static_cast<int>(out);
  int n_points_per_cluster = 5;
  int n_points = n_clusters*n_points_per_cluster;
  std::size_t row = 0;
  matrix_type A{static_cast<std::size_t>(n_points), static_cast<std::size_t>(n_dimensions)};

  // Trivial case: all dimensions are negative
  int val_magnitude = 50;
  std::vector<int> dimension_signs (static_cast<std::size_t>(n_dimensions), -1);
  for (std::size_t p=0; p < static_cast<std::size_t>(n_points_per_cluster); ++p){
    for (std::size_t k=0; k<static_cast<std::size_t>(n_dimensions); ++k)
    {
      int sign = dimension_signs[k];
      int val = val_magnitude * sign;
      A.Set(row, k, val);
    }
    row = row + 1;
    val_magnitude += 5;
  }

  //
  for (int r = 1; r <= n_dimensions; ++r){
    matrix_type combinations_array = combinations(n_dimensions, r);

    for (std::size_t i = 0; i < combinations_array.shape()[0]; ++i){
      std::vector<int> dimension_signs (static_cast<std::size_t>(n_dimensions), -1);
      for (std::size_t j = 0; j < combinations_array.shape()[1]; ++j){
        int positive_dimension = static_cast<int>(combinations_array.At(i,j)) - 1;
        dimension_signs[static_cast<std::size_t>(positive_dimension)] = 1;
      }
      std::cout << std::endl;
      int val_magnitude = 50;
      for (std::size_t p=0; p < static_cast<std::size_t>(n_points_per_cluster); ++p){
        for (std::size_t k=0; k<static_cast<std::size_t>(n_dimensions); ++k)
        {
          int sign = dimension_signs[k];
          int val = val_magnitude * sign;
          A.Set(row, k, val);
        }
        row = row + 1;
        val_magnitude += 5;
      }

    }
    std::cout << std::endl << "next starts at row: " << row << " of " << n_points << std::endl;
  }
  for (std::size_t l = 0; l < A.shape()[0]; ++l) {
    for (std::size_t m = 0; m < A.shape()[1]; ++m)
    {
      std::cout << A.At(l,m) << "\t\t";
    }
    std::cout << std::endl;
  }

  matrix_type combinations_array = combinations(n_dimensions, 0);
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
  matrix_type clusters    = fetch::math::clustering::KMeans(A, random_seed, prev_k, K);

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

TEST(clustering_test, kmeans_test_previous_assignment_no_K)
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

  std::size_t random_seed = 123456;
  matrix_type clusters    = fetch::math::clustering::KMeans(A, random_seed, prev_k);

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
