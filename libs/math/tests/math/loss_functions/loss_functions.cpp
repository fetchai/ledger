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

// #include <algorithm>
#include <iomanip>
#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

#include "math/loss_functions/loss_functions.hpp"
#include "math/ndarray.hpp"
#include "math/shape_less_array.hpp"

//#include "math/free_functions/free_functions.hpp"

using namespace fetch::math;
using T = double;
#define ARRAY_SIZE 100

TEST(loss_functions, MSE_Test)
{

  ShapeLessArray<T> y_arr     = ShapeLessArray<T>::UniformRandom(ARRAY_SIZE);
  ShapeLessArray<T> y_hat_arr = ShapeLessArray<T>::UniformRandom(ARRAY_SIZE);

  T result      = -999;
  T test_result = -999;

  ShapeLessArray<T> test_result_arr = ShapeLessArray<T>(y_arr.size());
  // error
  Subtract(y_arr, y_hat_arr, test_result_arr);
  // square
  Square(test_result_arr, test_result_arr);
  // mean
  Mean(test_result_arr, test_result);

  // the function we're testing
  ops::loss_functions::MeanSquareError(y_arr, y_hat_arr, result);

  ASSERT_TRUE(result == test_result);
}
