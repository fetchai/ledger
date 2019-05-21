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
#include <iostream>

#include "math/tensor.hpp"
#include "ml/clustering/tsne.hpp"

using namespace fetch::math;
using namespace fetch::math::distance;

template <typename T>
class TsneTests : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(TsneTests, MyTypes);

template <typename TypeParam>
TypeParam RunTest(typename TypeParam::SizeType n_data_size,
                  typename TypeParam::SizeType n_output_feature_size)
{

  using DataType     = typename TypeParam::Type;
  using ArrayType    = TypeParam;
  using SizeTypeHere = typename TypeParam::SizeType;

  SizeTypeHere RANDOM_SEED{123456};
  DataType     LEARNING_RATE{500};  // (seems very high!)
  SizeTypeHere MAX_ITERATIONS{1};
  DataType     PERPLEXITY{20};
  SizeTypeHere N_DATA_SIZE{n_data_size};
  SizeTypeHere N_INPUT_FEATURE_SIZE{3};
  SizeTypeHere N_OUTPUT_FEATURE_SIZE{n_output_feature_size};
  DataType     INITIAL_MOMENTUM{0.5f};
  DataType     FINAL_MOMENTUM{0.8f};
  SizeTypeHere FINAL_MOMENTUM_STEPS{20};
  SizeTypeHere P_LATER_CORRECTION_ITERATION{10};

  ArrayType A({N_DATA_SIZE, N_INPUT_FEATURE_SIZE});

  // Generate easily separable clusters of data
  for (SizeTypeHere i = 0; i < 25; ++i)
  {
    A.Set(i, SizeTypeHere{0}, -static_cast<DataType>(i) - static_cast<DataType>(50));
    A.Set(i, SizeTypeHere{1}, -static_cast<DataType>(i) - static_cast<DataType>(50));
    A.Set(i, SizeTypeHere{2}, -static_cast<DataType>(i) - static_cast<DataType>(50));
  }
  for (SizeTypeHere i = 25; i < 50; ++i)
  {
    A.Set(i, SizeTypeHere{0}, -static_cast<DataType>(i) - static_cast<DataType>(50));
    A.Set(i, SizeTypeHere{1}, static_cast<DataType>(i) + static_cast<DataType>(50));
    A.Set(i, SizeTypeHere{2}, static_cast<DataType>(i) + static_cast<DataType>(50));
  }
  for (SizeTypeHere i = 50; i < 75; ++i)
  {
    A.Set(i, SizeTypeHere{0}, static_cast<DataType>(i) + static_cast<DataType>(50));
    A.Set(i, SizeTypeHere{1}, -static_cast<DataType>(i) - static_cast<DataType>(50));
    A.Set(i, SizeTypeHere{2}, -static_cast<DataType>(i) - static_cast<DataType>(50));
  }
  for (SizeTypeHere i = 75; i < 100; ++i)
  {
    A.Set(i, SizeTypeHere{0}, static_cast<DataType>(i) + static_cast<DataType>(50));
    A.Set(i, SizeTypeHere{1}, static_cast<DataType>(i) + static_cast<DataType>(50));
    A.Set(i, SizeTypeHere{2}, static_cast<DataType>(i) + static_cast<DataType>(50));
  }

  fetch::ml::TSNE<ArrayType> tsn(A, N_OUTPUT_FEATURE_SIZE, PERPLEXITY, RANDOM_SEED);

  tsn.Optimize(LEARNING_RATE, MAX_ITERATIONS, INITIAL_MOMENTUM, FINAL_MOMENTUM,
               FINAL_MOMENTUM_STEPS, P_LATER_CORRECTION_ITERATION);
  return tsn.GetOutputMatrix();
}

TEST(TsneTests, tsne_test_2d_float)
{
  SizeType N_DATA_SIZE{100};
  SizeType N_OUTPUT_FEATURE_SIZE{2};

  Tensor<float> output_matrix = RunTest<Tensor<float>>(N_DATA_SIZE, N_OUTPUT_FEATURE_SIZE);

  ASSERT_EQ(output_matrix.shape().at(0), N_DATA_SIZE);
  ASSERT_EQ(output_matrix.shape().at(1), N_OUTPUT_FEATURE_SIZE);

  EXPECT_FLOAT_EQ(output_matrix.At(0, 0), 0.25323879718780517578);
  EXPECT_FLOAT_EQ(output_matrix.At(0, 1), -3.1758825778961181641);
  EXPECT_FLOAT_EQ(output_matrix.At(25, 0), -1.7577359676361083984);
  EXPECT_FLOAT_EQ(output_matrix.At(25, 1), 2.6265761852264404297);
  EXPECT_FLOAT_EQ(output_matrix.At(50, 0), 0.2358369976282119751);
  EXPECT_FLOAT_EQ(output_matrix.At(50, 1), 1.679751276969909668);
  EXPECT_FLOAT_EQ(output_matrix.At(99, 0), -0.87262111902236938477);
  EXPECT_FLOAT_EQ(output_matrix.At(99, 1), 3.0463356971740722656);
}

TEST(TsneTests, tsne_test_2d_double)
{
  SizeType N_DATA_SIZE{100};
  SizeType N_OUTPUT_FEATURE_SIZE{2};

  Tensor<double> output_matrix = RunTest<Tensor<double>>(N_DATA_SIZE, N_OUTPUT_FEATURE_SIZE);

  ASSERT_EQ(output_matrix.shape().at(0), N_DATA_SIZE);
  ASSERT_EQ(output_matrix.shape().at(1), N_OUTPUT_FEATURE_SIZE);

  EXPECT_DOUBLE_EQ(output_matrix.At(0, 0), 0.25323926146043396201);
  EXPECT_DOUBLE_EQ(output_matrix.At(0, 1), -3.1758751208173934266);
  EXPECT_DOUBLE_EQ(output_matrix.At(25, 0), -1.7577317050493974637);
  EXPECT_DOUBLE_EQ(output_matrix.At(25, 1), 2.6265693658422666346);
  EXPECT_DOUBLE_EQ(output_matrix.At(50, 0), 0.23583728299026301967);
  EXPECT_DOUBLE_EQ(output_matrix.At(50, 1), 1.6797469776066187297);
  EXPECT_DOUBLE_EQ(output_matrix.At(99, 0), -0.87261847085526600409);
  EXPECT_DOUBLE_EQ(output_matrix.At(99, 1), 3.0463283985051647917);
}

TEST(TsneTests, tsne_test_2d_fixed_point)
{
  using DataType = typename fetch::fixed_point::FixedPoint<32, 32>;
  SizeType N_DATA_SIZE{100};
  SizeType N_OUTPUT_FEATURE_SIZE{2};

  Tensor<DataType> output_matrix = RunTest<Tensor<DataType>>(N_DATA_SIZE, N_OUTPUT_FEATURE_SIZE);

  ASSERT_EQ(output_matrix.shape().at(0), N_DATA_SIZE);
  ASSERT_EQ(output_matrix.shape().at(1), N_OUTPUT_FEATURE_SIZE);

  EXPECT_NEAR(double(output_matrix.At(0, 0)), 0.25323880254290997982025146484375, 2e-6);
  EXPECT_NEAR(double(output_matrix.At(0, 1)), -3.17587922653183341026306152343750, 3e-6);
  EXPECT_NEAR(double(output_matrix.At(25, 0)), -1.75773554784245789051055908203125, 2e-6);
  EXPECT_NEAR(double(output_matrix.At(25, 1)), 2.62657403736375272274017333984375, 4e-6);
  EXPECT_NEAR(double(output_matrix.At(50, 0)), 0.23583724093623459339141845703125, 2e-6);
  EXPECT_NEAR(double(output_matrix.At(50, 1)), 1.67974892887286841869354248046875, 1e-6);
  EXPECT_NEAR(double(output_matrix.At(99, 0)), -0.87261968106031417846679687500000, 1e-6);
  EXPECT_NEAR(double(output_matrix.At(99, 1)), 3.04633120773360133171081542968750, 2e-6);
}

TYPED_TEST(TsneTests, tsne_test_2d_cross_type_consistency_test)
{
  SizeType N_DATA_SIZE{100};
  SizeType N_OUTPUT_FEATURE_SIZE{2};

  TypeParam output_matrix = RunTest<TypeParam>(N_DATA_SIZE, N_OUTPUT_FEATURE_SIZE);

  ASSERT_EQ(output_matrix.shape().at(0), N_DATA_SIZE);
  ASSERT_EQ(output_matrix.shape().at(1), N_OUTPUT_FEATURE_SIZE);

  EXPECT_NEAR(double(output_matrix.At(0, 0)), 0.25323879718780517578, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(0, 1)), -3.1758825778961181641, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(25, 0)), -1.7577359676361083984, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(25, 1)), 2.6265761852264404297, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(50, 0)), 0.2358369976282119751, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(50, 1)), 1.679751276969909668, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(99, 0)), -0.87262111902236938477, 1e-4);
  EXPECT_NEAR(double(output_matrix.At(99, 1)), 3.0463356971740722656, 1e-4);
}
