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

#include "math/computation_graph/computation_graph.hpp"
#include "math/ndarray.hpp"

using namespace fetch::math::computation_graph;

using T          = double;
using ARRAY_TYPE = fetch::math::NDArray<T>;

TEST(computation_graph, simple_arithmetic)
{
  double                          result;
  ComputationGraph<T, ARRAY_TYPE> computation_graph;

  computation_graph.ParseExpression("1 + 2");
  computation_graph.Run(result);
  ASSERT_TRUE(result == double(3));

  computation_graph.Reset();
  computation_graph.ParseExpression("1 - 2");
  computation_graph.Run(result);
  ASSERT_TRUE(result == double(-1));

  computation_graph.Reset();
  computation_graph.ParseExpression("1 * 2");
  computation_graph.Run(result);
  ASSERT_TRUE(result == double(2));

  computation_graph.Reset();
  computation_graph.ParseExpression("1 / 2");
  computation_graph.Run(result);
  ASSERT_TRUE(result == double(0.5));
}

TEST(computation_graph, multi_parenthesis_test)
{
  double                          result;
  ComputationGraph<T, ARRAY_TYPE> computation_graph;

  computation_graph.ParseExpression("(((1 + 2)))");
  computation_graph.Run(result);
  ASSERT_TRUE(result == double(3));

  computation_graph.Reset();
  computation_graph.ParseExpression("((1 - 2) * (3 / 4))");
  computation_graph.Run(result);
  ASSERT_TRUE(result == double(-0.75));
}

TEST(computation_graph, odd_num_nodes)
{
  double                          result;
  ComputationGraph<T, ARRAY_TYPE> computation_graph;

  computation_graph.ParseExpression("4 * 6 / 3");
  computation_graph.Run(result);
  ASSERT_TRUE(result == double(8));
}

TEST(computation_graph, multi_digit_nums)
{
  double                          result;
  ComputationGraph<T, ARRAY_TYPE> computation_graph;

  computation_graph.ParseExpression("100 * 62 / 31");
  computation_graph.Run(result);
  ASSERT_TRUE(result == double(200));
}

TEST(computation_graph, decimal_place_nums)
{
  double                          result;
  ComputationGraph<T, ARRAY_TYPE> computation_graph;

  computation_graph.ParseExpression("10.0 * 62.5 / 31.25");
  computation_graph.Run(result);
  ASSERT_TRUE(result == double(20));
}

TEST(computation_graph, ndarray_add)
{
  fetch::math::NDArray<double> arr1{10};
  fetch::math::NDArray<double> arr2{10};
  fetch::math::NDArray<double> result_arr{10};
  fetch::math::NDArray<double> test_result_arr{10};
  arr1.FillArange(0, 10);
  arr2.FillArange(0, 10);
  for (std::size_t i = 0; i < 10; ++i)
  {
    test_result_arr[i] = i + i;
  }
  ComputationGraph<T, ARRAY_TYPE> computation_graph;

  computation_graph.RegisterArray(arr1, "x");
  computation_graph.RegisterArray(arr2, "y");

  computation_graph.ParseExpression("x + y");
  computation_graph.Run(result_arr);
  ASSERT_TRUE(result_arr == test_result_arr);
}

TEST(computation_graph, ndarray_multiply)
{

  fetch::math::NDArray<double> arr1{25};
  arr1.Reshape({5, 5});
  fetch::math::NDArray<double> arr2{25};
  arr2.Reshape({5, 5});
  fetch::math::NDArray<double> result_arr{25};
  result_arr.Reshape({5, 5});
  fetch::math::NDArray<double> test_result_arr{25};
  test_result_arr.Reshape({5, 5});

  arr1.FillArange(0, 25);
  arr2.FillArange(0, 25);
  for (std::size_t i = 0; i < 25; ++i)
  {
    test_result_arr[i] = i * i;
  }
  ComputationGraph<T, ARRAY_TYPE> computation_graph;

  computation_graph.RegisterArray(arr1, "x");
  computation_graph.RegisterArray(arr2, "y");

  computation_graph.ParseExpression("x * y");
  computation_graph.Run(result_arr);
  ASSERT_TRUE(result_arr == test_result_arr);
}

TEST(computation_graph, ndarray_tricky)
{

  fetch::math::NDArray<double> arr1{5};
  arr1.Reshape({1, 5});
  fetch::math::NDArray<double> arr2{25};
  arr2.Reshape({5, 5});
  fetch::math::NDArray<double> arr3{5};
  arr3.Reshape({5, 1});
  fetch::math::NDArray<double> result_arr{25};
  result_arr.Reshape({5, 5});

  fetch::math::NDArray<double> test_arr1{5};
  test_arr1.Reshape({1, 5});
  fetch::math::NDArray<double> test_arr2{25};
  test_arr2.Reshape({5, 5});
  fetch::math::NDArray<double> test_arr3{5};
  test_arr3.Reshape({5, 1});
  fetch::math::NDArray<double> test_result_arr{25};
  test_result_arr.Reshape({5, 5});

  arr1.FillArange(0, 5);
  fetch::math::Add(arr1, 1.0, arr1);
  arr2.FillArange(0, 25);
  fetch::math::Add(arr2, 1.0, arr2);
  arr3.FillArange(0, 5);
  fetch::math::Add(arr3, 1.0, arr3);

  test_arr1.FillArange(0, 5);
  fetch::math::Add(test_arr1, 1.0, test_arr1);
  test_arr2.FillArange(0, 25);
  fetch::math::Add(test_arr2, 1.0, test_arr2);
  test_arr3.FillArange(0, 5);
  fetch::math::Add(test_arr3, 1.0, test_arr3);

  // run standard array computations
  test_result_arr = (fetch::math::Multiply(test_arr1, test_arr3));
  test_result_arr = (fetch::math::Divide(test_result_arr, test_arr2));

  // run on computation_graph
  ComputationGraph<T, ARRAY_TYPE> computation_graph;

  computation_graph.RegisterArray(arr1, "x");
  computation_graph.RegisterArray(arr2, "y");
  computation_graph.RegisterArray(arr3, "z");

  computation_graph.ParseExpression("(x * z) / y");
  computation_graph.Run(result_arr);

  ASSERT_TRUE(result_arr == test_result_arr);
}

// TEST(computation_graph, back_prop)
//{
//
//  fetch::math::NDArray<T> arr1 = fetch::math::NDArray<T>::UniformRandom(100);
//  fetch::math::NDArray<T> arr2 = fetch::math::NDArray<T>::UniformRandom(100);
//  fetch::math::NDArray<T> y = fetch::math::NDArray<T>::UniformRandom(100);
//  fetch::math::NDArray<T> y_hat{100};
//
//
//  T result = -999;
//  T test_result = -999;
//
//  // run on computation_graph
//  ComputationGraph<T, ARRAY_TYPE> computation_graph;
//
//  computation_graph.RegisterArray(arr1, "x");
//  computation_graph.RegisterArray(arr2, "y");
//
//  computation_graph.ParseExpression("(x * y)");
//  computation_graph.Run(y_hat);
//
//  computation_graph.SetLoss("MSE");
//  computation_graph.Train();
//
//  computation_graph.Train();
//
//  ASSERT_TRUE(result_arr == test_result_arr);
//}
