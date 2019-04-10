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

template <typename T>
class TsneTests : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(TsneTests, MyTypes);

// TODO: re-enable test after broadcasting fix

TYPED_TEST(TsneTests, tsne_test_2d_DISABLED)
{
  /*
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  SizeType RANDOM_SEED{123456};
  DataType LEARNING_RATE{500};  // (seems very high!)
  SizeType MAX_ITERATIONS{1};
  DataType PERPLEXITY{20};
  SizeType N_DATA_SIZE{100};
  SizeType N_INPUT_FEATURE_SIZE{3};
  SizeType N_OUTPUT_FEATURE_SIZE{2};
  DataType INITIAL_MOMENTUM{0.5f};
  DataType FINAL_MOMENTUM{0.8f};
  SizeType FINAL_MOMENTUM_STEPS{20};
  SizeType P_LATER_CORRECTION_ITERATION{10};

  ArrayType A({N_DATA_SIZE, N_INPUT_FEATURE_SIZE});

  // Generate easily separable clusters of data
  for (SizeType i = 0; i < 25; ++i)
  {
    A.Set({i, 0}, -static_cast<DataType>(i) - static_cast<DataType>(50));
    A.Set({i, 1}, -static_cast<DataType>(i) - static_cast<DataType>(50));
    A.Set({i, 2}, -static_cast<DataType>(i) - static_cast<DataType>(50));
  }
  for (SizeType i = 25; i < 50; ++i)
  {
    A.Set({i, 0}, -static_cast<DataType>(i) - static_cast<DataType>(50));
    A.Set({i, 1}, static_cast<DataType>(i) + static_cast<DataType>(50));
    A.Set({i, 2}, static_cast<DataType>(i) + static_cast<DataType>(50));
  }
  for (SizeType i = 50; i < 75; ++i)
  {
    A.Set({i, 0}, static_cast<DataType>(i) + static_cast<DataType>(50));
    A.Set({i, 1}, -static_cast<DataType>(i) - static_cast<DataType>(50));
    A.Set({i, 2}, -static_cast<DataType>(i) - static_cast<DataType>(50));
  }
  for (SizeType i = 75; i < 100; ++i)
  {
    A.Set({i, 0}, static_cast<DataType>(i) + static_cast<DataType>(50));
    A.Set({i, 1}, static_cast<DataType>(i) + static_cast<DataType>(50));
    A.Set({i, 2}, static_cast<DataType>(i) + static_cast<DataType>(50));
  }

  fetch::ml::TSNE<ArrayType> tsn(A, N_OUTPUT_FEATURE_SIZE, PERPLEXITY, RANDOM_SEED);

  tsn.Optimize(LEARNING_RATE, MAX_ITERATIONS, INITIAL_MOMENTUM, FINAL_MOMENTUM,
               FINAL_MOMENTUM_STEPS, P_LATER_CORRECTION_ITERATION);
  ArrayType output_matrix = tsn.GetOutputMatrix();
  ASSERT_EQ(output_matrix.shape().at(0), N_DATA_SIZE);
  ASSERT_EQ(output_matrix.shape().at(1), N_OUTPUT_FEATURE_SIZE);

  EXPECT_NEAR(double(output_matrix.At(0, 0)), -0.26619809865951538, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(0, 1)), -0.60562509298324585, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(25, 0)), 0.34291094541549683, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(25, 1)), 1.8279048204421997, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(50, 0)), -4.390531063079834, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(50, 1)), -0.99320822954177856, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(99, 0)), 2.1976754665374756, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(99, 1)), 2.5504453182220459, 1e-4);

   */
}
