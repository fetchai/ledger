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

#include "ml/clustering/tsne.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <math/tensor.hpp>

using namespace fetch::math;
using namespace fetch::math::distance;

using DataType  = double;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

TEST(tsne_tests, tsne_test_2d)
{
  SizeType n_data_size           = 100;
  SizeType n_input_feature_size  = 3;
  SizeType n_output_feature_size = 2;

  ArrayType A({n_data_size, n_input_feature_size});
  ArrayType ret({n_data_size, n_output_feature_size});

  for (SizeType i = 0; i < 25; ++i)
  {
    A.Set({i, 0}, -static_cast<DataType>(i) - 50);
    A.Set({i, 1}, -static_cast<DataType>(i) - 50);
    A.Set({i, 2}, -static_cast<DataType>(i) - 50);
  }
  for (SizeType i = 25; i < 50; ++i)
  {
    A.Set({i, 0}, -static_cast<DataType>(i) - 50);
    A.Set({i, 1}, static_cast<DataType>(i) + 50);
    A.Set({i, 2}, static_cast<DataType>(i) + 50);
  }
  for (SizeType i = 50; i < 75; ++i)
  {
    A.Set({i, 0}, static_cast<DataType>(i) + 50);
    A.Set({i, 1}, -static_cast<DataType>(i) - 50);
    A.Set({i, 2}, -static_cast<DataType>(i) - 50);
  }
  for (SizeType i = 75; i < 100; ++i)
  {
    A.Set({i, 0}, static_cast<DataType>(i) + 50);
    A.Set({i, 1}, static_cast<DataType>(i) + 50);
    A.Set({i, 2}, static_cast<DataType>(i) + 50);
  }

  SizeType                        random_seed = 123456;
  fetch::ml::TSNE<Tensor<double>> tsn(A, n_output_feature_size, random_seed);

  tsn.Optimize(500, 30);
  ArrayType output_matrix = tsn.GetOutputMatrix();
  ASSERT_EQ(output_matrix.shape().at(0), n_data_size);
  ASSERT_EQ(output_matrix.shape().at(1), n_output_feature_size);

  EXPECT_NEAR(output_matrix.At({0, 0}), -1.064013785364649, 1e-7);
  EXPECT_NEAR(output_matrix.At({0, 1}), -7.6949466236324255, 1e-7);
  EXPECT_NEAR(output_matrix.At({25, 0}), 7.2096322015170999, 1e-7);
  EXPECT_NEAR(output_matrix.At({25, 1}), 1.3472895208253279, 1e-7);
  EXPECT_NEAR(output_matrix.At({50, 0}), -0.66978511917211891, 1e-7);
  EXPECT_NEAR(output_matrix.At({50, 1}), 6.332879767033762, 1e-7);
  EXPECT_NEAR(output_matrix.At({99, 0}), -7.4490467472508737, 1e-7);
  EXPECT_NEAR(output_matrix.At({99, 1}), -1.076941080720619, 1e-7);
}
