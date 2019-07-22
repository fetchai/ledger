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

#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/utilities/min_max_scaler.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class ScalerTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(ScalerTest, MyTypes);

TYPED_TEST(ScalerTest, min_max_2d_test)
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename fetch::math::SizeType;

  std::vector<SizeType> tensor_shape = {2, 4};

  TypeParam data(tensor_shape);
  data.FillUniformRandom();

  // multiplication to push data outside of 0-1 range
  data *= static_cast<DataType>(1000);

  TypeParam norm_data(tensor_shape);
  TypeParam de_norm_data(tensor_shape);

  fetch::ml::utilities::MinMaxScaler<TypeParam> scaler;
  scaler.SetScale(data);

  scaler.Normalise(data, norm_data);
  scaler.DeNormalise(norm_data, de_norm_data);

  EXPECT_EQ(data.shape(), norm_data.shape());
  EXPECT_EQ(de_norm_data.shape(), norm_data.shape());

  EXPECT_TRUE(data.AllClose(de_norm_data, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(fetch::math::Max(norm_data) <= static_cast<DataType>(1));
  EXPECT_TRUE(fetch::math::Min(norm_data) >= static_cast<DataType>(0));
}

TYPED_TEST(ScalerTest, min_max_3d_test)
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename fetch::math::SizeType;

  std::vector<SizeType> tensor_shape = {2, 3, 4};

  TypeParam data(tensor_shape);
  data.FillUniformRandom();

  TypeParam norm_data(tensor_shape);
  TypeParam de_norm_data(tensor_shape);

  fetch::ml::utilities::MinMaxScaler<TypeParam> scaler;
  scaler.SetScale(data);

  scaler.Normalise(data, norm_data);
  scaler.DeNormalise(norm_data, de_norm_data);

  EXPECT_EQ(data.shape(), norm_data.shape());
  EXPECT_EQ(de_norm_data.shape(), norm_data.shape());

  EXPECT_TRUE(data.AllClose(de_norm_data, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(fetch::math::Max(norm_data) <= static_cast<DataType>(1));
  EXPECT_TRUE(fetch::math::Min(norm_data) >= static_cast<DataType>(0));
}
