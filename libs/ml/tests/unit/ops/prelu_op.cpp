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

#include "math/base_types.hpp"
#include "ml/ops/prelu_op.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace {

using namespace fetch::ml;

template <typename T>
class PReluOpTest : public ::testing::Test
{
};

TYPED_TEST_CASE(PReluOpTest, fetch::math::test::TensorFloatingTypes);

TYPED_TEST(PReluOpTest, forward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType data =
      TensorType::FromString("1, -2, 3,-4, 5,-6, 7,-8; -1,  2,-3, 4,-5, 6,-7, 8").Transpose();

  TensorType gt =
      TensorType::FromString(
          "1,-0.4,   3,-1.6,   5,-3.6,   7,-6.4; -0.1,   2,-0.9,   4,-2.5,   6,-4.9,   8")
          .Transpose();

  TensorType alpha = TensorType::FromString("0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8").Transpose();

  fetch::ml::ops::PReluOp<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}));
  op.Forward({std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(PReluOpTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType alpha =
      TensorType::FromString(R"(0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8)").Transpose();

  TensorType data = TensorType::FromString(R"(1,-2, 3,-4, 5,-6, 7,-8;
                                             -1, 2,-3, 4,-5, 6,-7, 8)")
                        .Transpose();

  TensorType gt = TensorType::FromString(R"(0, 0, 0, 0, 1, 0.6, 0, 0;
                                            0, 0, 0, 0, 0.5, 1, 0, 0)")
                      .Transpose();

  TensorType error = TensorType::FromString(R"(0, 0, 0, 0, 1, 1, 0, 0;
                                               0, 0, 0, 0, 1, 1, 0, 0)")
                         .Transpose();

  fetch::ml::ops::PReluOp<TensorType> op;
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<TypeParam>(data), std::make_shared<TypeParam>(alpha)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

}  // namespace
