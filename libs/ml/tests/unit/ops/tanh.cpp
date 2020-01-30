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
#include "ml/ops/tanh.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace {

using namespace fetch::ml;

template <typename T>
class TanHTest : public ::testing::Test
{
};

TYPED_TEST_CASE(TanHTest, fetch::math::test::TensorFloatingTypes);

TYPED_TEST(TanHTest, forward_all_positive_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  uint8_t   n      = 9;
  TypeParam data{n};
  TypeParam gt({n});

  TensorType data_input = TensorType::FromString("0, 0.2, 0.4, 0.6, 0.8, 1, 1.2, 1.4, 10");
  TensorType gt_input   = TensorType::FromString(
      "0.0, 0.197375, 0.379949, 0.53705, 0.664037, 0.761594, 0.833655, 0.885352, 1.0");

  for (uint64_t i(0); i < n; ++i)
  {
    data.Set(i, data_input[i]);
    gt.Set(i, gt_input[i]);
  }

  fetch::ml::ops::TanH<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TypeParam>(data)}));
  op.Forward({std::make_shared<const TypeParam>(data)}, prediction);

  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>() * DataType{5},
                                  fetch::math::function_tolerance<DataType>() * DataType{5}));
}

TYPED_TEST(TanHTest, forward_all_negative_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  uint8_t   n      = 9;
  TypeParam data{n};
  TypeParam gt{n};

  TensorType data_input = TensorType::FromString("-0, -0.2, -0.4, -0.6, -0.8, -1, -1.2, -1.4, -10");
  TensorType gt_input   = TensorType::FromString(
      "-0.0, -0.197375, -0.379949, -0.53705, -0.664037, -0.761594, -0.833655, -0.885352, -1.0");

  for (uint64_t i(0); i < n; ++i)
  {
    data.Set(i, data_input[i]);
    gt.Set(i, gt_input[i]);
  }

  fetch::ml::ops::TanH<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TypeParam>(data)}));
  op.Forward({std::make_shared<const TypeParam>(data)}, prediction);

  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>() * DataType{5},
                                  fetch::math::function_tolerance<DataType>() * DataType{5}));
}

TYPED_TEST(TanHTest, backward_all_positive_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  uint8_t    n     = 8;
  TypeParam  data{n};
  TypeParam  error{n};
  TypeParam  gt{n};
  TensorType data_input  = TensorType::FromString("0, 0.2, 0.4, 0.6, 0.8, 1.2, 1.4, 10");
  TensorType error_input = TensorType::FromString("0.2, 0.1, 0.3, 0.2, 0.5, 0.1, 0.0, 0.3");
  TensorType gt_input =
      TensorType::FromString("0.2, 0.096104, 0.256692, 0.142316, 0.279528, 0.030502, 0.0, 0.0");
  for (uint64_t i(0); i < n; ++i)
  {
    data.Set(i, data_input[i]);
    error.Set(i, error_input[i]);
    gt.Set(i, gt_input[i]);
  }
  fetch::ml::ops::TanH<TypeParam> op;
  std::vector<TypeParam> prediction = op.Backward({std::make_shared<const TypeParam>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>() * DataType{5},
                                     fetch::math::function_tolerance<DataType>() * DataType{5}));
}

TYPED_TEST(TanHTest, backward_all_negative_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  uint8_t    n     = 8;
  TypeParam  data{n};
  TypeParam  error{n};
  TypeParam  gt{n};
  TensorType data_input  = TensorType::FromString("-0, -0.2, -0.4, -0.6, -0.8, -1.2, -1.4, -10");
  TensorType error_input = TensorType::FromString("-0.2, -0.1, -0.3, -0.2, -0.5, -0.1, -0.0, -0.3");
  TensorType gt_input    = TensorType::FromString(
      "-0.2, -0.096104, -0.256692, -0.142316, -0.279528, -0.030502, 0.0, 0.0");
  for (uint64_t i(0); i < n; ++i)
  {
    data.Set(i, data_input[i]);
    error.Set(i, error_input[i]);
    gt.Set(i, gt_input[i]);
  }
  fetch::ml::ops::TanH<TypeParam> op;
  std::vector<TypeParam> prediction = op.Backward({std::make_shared<const TypeParam>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>() * DataType{5},
                                     fetch::math::function_tolerance<DataType>() * DataType{5}));
}

}  // namespace
