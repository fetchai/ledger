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

#include "core/random/lcg.hpp"
#include "math/free_functions/free_functions.hpp"
#include <math/linalg/matrix.hpp>

using namespace fetch::math::linalg;

using data_type            = double;
using container_type       = fetch::memory::SharedArray<data_type>;
using matrix_type          = Matrix<data_type, container_type>;
using vector_register_type = typename matrix_type::vector_register_type;

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _M = Matrix<D, _S<D>>;

// Test sigmoid function output against numpy output for 2x2 input matrix of random values
TEST(free_functions, sigmoid_test_2x2)
{
  matrix_type array1{2, 2};

  array1.Set(0, 0, 0.3);
  array1.Set(0, 1, 1.2);
  array1.Set(1, 0, 0.7);
  array1.Set(1, 1, 22);

  matrix_type output = fetch::math::Sigmoid(array1);

  matrix_type numpy_output = matrix_type(R"(
	0.57444252 0.76852478;
 0.66818777 1
	)");

  ASSERT_TRUE(output.AllClose(numpy_output));
}

// Test sigmoid function output against numpy output for 1x1 input matrix of random values
TEST(free_functions, sigmoid_test_1x1)
{
  matrix_type input{1, 1};
  matrix_type output{1, 1};
  matrix_type numpy_output{1, 1};

  for (std::size_t i = 0; i < input.size(); ++i)
  {
    input.Set(i, 0);
    output.Set(i, 0);
    numpy_output.Set(i, 0);
  }

  input.Set(0, 0, 0.3);

  matrix_type out = fetch::math::Sigmoid(input);
  output.Set(0, out.At(0));

  numpy_output.Set(0, 0.574442516811659);

  ASSERT_TRUE(output.AllClose(numpy_output));
}

// Test tanh function output against numpy output for 2x2 input matrix of random values
TEST(free_functions, tanh_test_2x2)
{
  matrix_type array1{2, 2};

  array1.Set(0, 0, 0.3);
  array1.Set(0, 1, 1.2);
  array1.Set(1, 0, 0.7);
  array1.Set(1, 1, 22);

  matrix_type output = fetch::math::Tanh(array1);

  matrix_type numpy_output = matrix_type(R"(
	0.29131261 0.83365461;
 0.60436778 1.
	)");

  ASSERT_TRUE(output.AllClose(numpy_output));
}