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
#include <math/linalg/matrix.hpp>

#include "core/random/lcg.hpp"
#include "math/free_functions/free_functions.hpp"
#include "math/ndarray.hpp"

///////////////////
/// Sigmoid 2x2 ///
///////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values

template <typename T, typename ArrayType>
void sigmoid_22()
{
  ArrayType array1{4};
  array1.Reshape({2, 2});

  array1[0] = T(0.3);
  array1[1] = T(1.2);
  array1[2] = T(0.7);
  array1[3] = T(22);

  ArrayType output = fetch::math::Sigmoid(array1);

  ArrayType numpy_output{4};
  numpy_output.Reshape({2, 2});

  numpy_output[0] = T(0.57444252);
  numpy_output[1] = T(0.76852478);
  numpy_output[2] = T(0.66818777);
  numpy_output[3] = T(1);

  ASSERT_TRUE(output.AllClose(numpy_output));
}
TEST(ndarray, sigmoid_2x2_matrix_float)
{
  sigmoid_22<float, fetch::math::linalg::Matrix<float>>();
}
TEST(ndarray, sigmoid_2x2_matrix_double)
{
  sigmoid_22<double, fetch::math::linalg::Matrix<double>>();
}
TEST(ndarray, sigmoid_2x2_ndarray_float)
{
  sigmoid_22<float, fetch::math::NDArray<float>>();
}
TEST(ndarray, sigmoid_2x2_ndarray_double)
{
  sigmoid_22<double, fetch::math::NDArray<double>>();
}

///////////////////
/// Sigmoid 1x1 ///
///////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values

template <typename T, typename ArrayType>
void sigmoid_11()
{
  ArrayType input{1};
  ArrayType output{1};
  ArrayType numpy_output{1};

  input[0]        = T(0.3);
  numpy_output[0] = 0;

  output = fetch::math::Sigmoid(input);

  numpy_output[0] = T(0.574442516811659);

  ASSERT_TRUE(output.AllClose(numpy_output));
}
TEST(ndarray, sigmoid_11_matrix_float)
{
  sigmoid_11<float, fetch::math::linalg::Matrix<float>>();
}
TEST(ndarray, sigmoid_11_matrix_double)
{
  sigmoid_11<double, fetch::math::linalg::Matrix<double>>();
}
TEST(ndarray, sigmoid_11_ndarray_float)
{
  sigmoid_11<float, fetch::math::NDArray<float>>();
}
TEST(ndarray, sigmoid_11_ndarray_double)
{
  sigmoid_11<double, fetch::math::NDArray<double>>();
}

////////////////
/// Tanh 2x2 ///
////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values

template <typename T, typename ArrayType>
void tanh_22()
{
  ArrayType array1{4};
  array1.Reshape({2, 2});

  array1[0] = T(0.3);
  array1[1] = T(1.2);
  array1[2] = T(0.7);
  array1[3] = T(22);

  ArrayType output = array1;
  fetch::math::Tanh(output);

  ArrayType numpy_output{4};
  array1.Reshape({2, 2});

  numpy_output[0] = T(0.29131261);
  numpy_output[1] = T(0.83365461);
  numpy_output[2] = T(0.60436778);
  numpy_output[3] = T(1);

  ASSERT_TRUE(output.AllClose(numpy_output));
}
TEST(ndarray, Tanh_22_matrix_float)
{
  tanh_22<float, fetch::math::linalg::Matrix<float>>();
}
TEST(ndarray, Tanh_22_matrix_double)
{
  tanh_22<double, fetch::math::linalg::Matrix<double>>();
}
TEST(ndarray, Tanh_22_ndarray_float)
{
  sigmoid_11<float, fetch::math::NDArray<float>>();
}
TEST(ndarray, Tanh_22_ndarray_double)
{
  sigmoid_11<double, fetch::math::NDArray<double>>();
}
