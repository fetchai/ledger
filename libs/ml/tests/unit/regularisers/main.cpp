//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "gtest/gtest.h"
#include "ml/ops/weights.hpp"
#include "ml/regularisers/l1_regulariser.hpp"
#include "ml/regularisers/l2_regulariser.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class WeightsTest : public ::testing::Test
{
};

TYPED_TEST_CASE(WeightsTest, math::test::TensorFloatingTypes);

TYPED_TEST(WeightsTest, allocation_test)
{
  fetch::ml::ops::Weights<TypeParam> w;
}

TYPED_TEST(WeightsTest, l1_regulariser_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using RegType    = fetch::ml::regularisers::L1Regulariser<TensorType>;

  // Initialise values
  auto regularisation_rate = fetch::math::Type<DataType>("0.1");
  auto regulariser         = std::make_shared<RegType>();

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType gt   = TensorType::FromString("0.9, -1.9, 2.9, -3.9, 4.9, -5.9, 6.9, -7.9");

  fetch::ml::ops::Weights<TensorType> w;
  w.SetData(data);

  // Apply regularisation
  w.SetRegularisation(regulariser, regularisation_rate);
  TensorType grad = w.GetGradients();
  grad.Fill(DataType{0});
  w.ApplyGradient(grad);

  // Evaluate weight
  TensorType prediction(w.ComputeOutputShape({}));
  w.Forward({}, prediction);

  // Test actual values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(WeightsTest, l2_regulariser_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using RegType    = fetch::ml::regularisers::L2Regulariser<TensorType>;

  // Initialise values
  auto regularisation_rate = fetch::math::Type<DataType>("0.1");
  auto regulariser         = std::make_shared<RegType>();

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType gt   = TensorType::FromString("0.8, -1.6, 2.4, -3.2, 4.0, -4.8, 5.6, -6.4");

  fetch::ml::ops::Weights<TensorType> w;
  w.SetData(data);

  // Apply regularisation
  w.SetRegularisation(regulariser, regularisation_rate);
  TensorType grad = w.GetGradients();
  grad.Fill(DataType{0});
  w.ApplyGradient(grad);

  // Evaluate weight
  TensorType prediction(w.ComputeOutputShape({}));
  w.Forward({}, prediction);

  // Test actual values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
