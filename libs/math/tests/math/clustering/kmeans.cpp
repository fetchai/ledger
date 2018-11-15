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
  int i;
  for(i = n-1; i > 1; i--)
    n *= i;

  return n;
}

// Helper function
matrix_type combinations(int n, int r)
{
  int n_combinations = factorial(n)/factorial(r)/factorial(n-r);
  int current_dim = 0;
  int current_row = 0;

  std::vector<bool> v(static_cast<unsigned long>(n));
  std::fill(v.end() - r, v.end(), true);

  matrix_type output_array{static_cast<std::size_t>(n_combinations), static_cast<std::size_t>(r)};
  do {
    for (int i = 0; i < n; ++i) {
      if (v[static_cast<unsigned long>(i)]) {
        if (current_dim == 0){
          int dim0 = (i + 1);
          // std::cout << "dim0: " << dim0 << " ";
          current_dim = current_dim + 1;
          current_dim = current_dim % r;
          output_array.At(static_cast<std::size_t>(current_row), 0) = dim0;
        }
        else if (current_dim == 1){
          int dim1 = (i + 1);
          // std::cout << "dim1: " << dim1 << " ";
          current_dim = current_dim + 1;
          current_dim = current_dim % r;
          output_array.At(static_cast<std::size_t>(current_row), 1) = dim1;
          current_row = current_row + 1;
        }
      }
    }
    // std::cout << "\n";
  } while (std::next_permutation(v.begin(), v.end()));
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
  int n_points = n_clusters*5;  // Each cluster will consist of 5 points
  matrix_type A{static_cast<std::size_t>(n_points), static_cast<std::size_t>(n_dimensions)};

  matrix_type com = combinations(4, 2);

  for (std::size_t i = 0; i < 6; ++i){
    for (std::size_t j = 0; j < 2; ++j){
      std::cout << com.At(i,j);
    }
    std::cout << std::endl;
  }


//
//  do {
//    for (int i = 0; i < n; ++i) {
//      if (v[static_cast<unsigned long>(i)]) {
//        std::cout << (i + 1) << " ";
//      }
//    }
//    std::cout << "\n";
//  } while (std::next_permutation(v.begin(), v.end()));




//  int n = 4;
//  int r = 2;
//  int n_combinations = factorial(n)/factorial(r)/factorial(n-r);
//  int which_dim = 0;
//  int row = 0;
//
//  std::vector<bool> v(static_cast<unsigned long>(n));
//  std::fill(v.end() - r, v.end(), true);
//
//  matrix_type output_array{static_cast<std::size_t>(n_combinations), static_cast<std::size_t>(r)};
//  do {
//    for (int i = 0; i < n; ++i) {
//      if (v[static_cast<unsigned long>(i)]) {
//        if (which_dim == 0){
//          int dim0 = (i + 1);
//          std::cout << "dim0: " << dim0 << " ";
//          which_dim = which_dim + 1;
//          which_dim = which_dim % r;
//          output_array.At(static_cast<std::size_t>(row), 0) = dim0;
//        }
//        else if (which_dim == 1){
//          int dim1 = (i + 1);
//          std::cout << "dim1: " << dim1 << " ";
//          which_dim = which_dim + 1;
//          which_dim = which_dim % r;
//          output_array.At(static_cast<std::size_t>(row), 1) = dim1;
//          row = row + 1;
//        }
//      }
//    }
//    std::cout << "\n";
//  } while (std::next_permutation(v.begin(), v.end()));
//
//  for (std::size_t i = 0; i < 6; ++i){
//    for (std::size_t j = 0; j < 2; ++j){
//      std::cout << output_array.At(i,j);
//    }
//    std::cout << std::endl;
//  }



  for(int c=0; c < n_clusters; ++c)
  {
    for (int p = 0; p < n_points; ++p)
    {

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
