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

#include "math/tensor.hpp"
#include "test_types.hpp"
#include "ml/clustering/tsne.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class TsneTests : public ::testing::Test
{
};

// we do not test for fp32 since that tends to overflow
TYPED_TEST_CASE(TsneTests, math::test::HighPrecisionTensorFloatingTypes);

template <typename TypeParam>
TypeParam RunTest(typename TypeParam::SizeType n_output_feature_size,
                  typename TypeParam::SizeType n_data_size)
{
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  SizeType RANDOM_SEED{123456};
  DataType LEARNING_RATE{500};  // (seems very high!)
  SizeType MAX_ITERATIONS{1};
  DataType PERPLEXITY{20};
  SizeType N_DATA_SIZE{n_data_size};
  SizeType N_INPUT_FEATURE_SIZE{3};
  SizeType N_OUTPUT_FEATURE_SIZE{n_output_feature_size};
  DataType INITIAL_MOMENTUM{0.5f};
  DataType FINAL_MOMENTUM{0.8f};
  SizeType FINAL_MOMENTUM_STEPS{20};
  SizeType P_LATER_CORRECTION_ITERATION{10};

  TensorType A({N_INPUT_FEATURE_SIZE, N_DATA_SIZE});

  // Generate easily separable clusters of data
  for (SizeType i = 0; i < 25; ++i)
  {
    A(0, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
    A(1, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
    A(2, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
  }
  for (SizeType i = 25; i < 50; ++i)
  {
    A(0, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
    A(1, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
    A(2, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
  }
  for (SizeType i = 50; i < 75; ++i)
  {
    A(0, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
    A(1, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
    A(2, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
  }
  for (SizeType i = 75; i < 100; ++i)
  {
    A(0, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
    A(1, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
    A(2, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
  }

  fetch::ml::TSNE<TensorType> tsn(A, N_OUTPUT_FEATURE_SIZE, PERPLEXITY, RANDOM_SEED);

  tsn.Optimise(LEARNING_RATE, MAX_ITERATIONS, INITIAL_MOMENTUM, FINAL_MOMENTUM,
               FINAL_MOMENTUM_STEPS, P_LATER_CORRECTION_ITERATION);
  return tsn.GetOutputMatrix();
}

TYPED_TEST(TsneTests, tsne_test_2d)
{
  using DataType = typename TypeParam::Type;
  math::SizeType n_data{100};
  math::SizeType n_features{2};

  TypeParam output_matrix = RunTest<TypeParam>(n_features, n_data);

  ASSERT_EQ(output_matrix.shape().at(0), n_features);
  ASSERT_EQ(output_matrix.shape().at(1), n_data);

  // in general we set the tolerance to be function tolerance * number of operations.
  // since tsne is a training procedure the number of operations is relatively large.
  // here we use 50 as proxy instead of calculating the number of operations
  // 50 is quite strict since there are 100 data points
  EXPECT_NEAR(double(output_matrix.At(0, 0)), 0.25323880254290997982025146484375, 50 * static_cast<double>(math::function_tolerance<DataType>()));
  EXPECT_NEAR(double(output_matrix.At(1, 0)), -3.17587922653183341026306152343750, 50 * static_cast<double>(math::function_tolerance<DataType>()));
  EXPECT_NEAR(double(output_matrix.At(0, 25)), -1.75773554784245789051055908203125, 50 * static_cast<double>(math::function_tolerance<DataType>()));
  EXPECT_NEAR(double(output_matrix.At(1, 25)), 2.62657403736375272274017333984375, 50 * static_cast<double>(math::function_tolerance<DataType>()));
  EXPECT_NEAR(double(output_matrix.At(0, 50)), 0.23583724093623459339141845703125, 50 * static_cast<double>(math::function_tolerance<DataType>()));
  EXPECT_NEAR(double(output_matrix.At(1, 50)), 1.67974892887286841869354248046875, 50 * static_cast<double>(math::function_tolerance<DataType>()));
  EXPECT_NEAR(double(output_matrix.At(0, 99)), -0.87261968106031417846679687500000, 50 * static_cast<double>(math::function_tolerance<DataType>()));
  EXPECT_NEAR(double(output_matrix.At(1, 99)), 3.04633120773360133171081542968750, 50 * static_cast<double>(math::function_tolerance<DataType>()));
}

TYPED_TEST(TsneTests, tsne_test_2d_cross_type_consistency_test)
{
  using DataType = typename TypeParam::Type;

  math::SizeType N_DATA_SIZE{100};
  math::SizeType N_OUTPUT_FEATURE_SIZE{2};

  TypeParam output_matrix = RunTest<TypeParam>(N_OUTPUT_FEATURE_SIZE, N_DATA_SIZE);

  ASSERT_EQ(output_matrix.shape().at(0), N_OUTPUT_FEATURE_SIZE);
  ASSERT_EQ(output_matrix.shape().at(1), N_DATA_SIZE);

  double tolerance = 60 * double(fetch::math::function_tolerance<DataType>());

  EXPECT_NEAR(double(output_matrix.At(0, 0)), 0.25323879718780517578, tolerance);
  EXPECT_NEAR(double(output_matrix.At(1, 0)), -3.1758825778961181641, tolerance);
  EXPECT_NEAR(double(output_matrix.At(0, 25)), -1.7577359676361083984, tolerance);
  EXPECT_NEAR(double(output_matrix.At(1, 25)), 2.6265761852264404297, tolerance);
  EXPECT_NEAR(double(output_matrix.At(0, 50)), 0.2358369976282119751, tolerance);
  EXPECT_NEAR(double(output_matrix.At(1, 50)), 1.679751276969909668, tolerance);
  EXPECT_NEAR(double(output_matrix.At(0, 99)), -0.87262111902236938477, tolerance);
  EXPECT_NEAR(double(output_matrix.At(1, 99)), 3.0463356971740722656, tolerance);
}

} // test
} // ml
} // fetch
