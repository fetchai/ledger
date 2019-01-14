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

#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "ml/ops/weights.hpp"
#include <gtest/gtest.h>

template <typename T>
class WeightsTest : public ::testing::Test
{
};

using MyTypes =
    ::testing::Types<fetch::math::NDArray<int>, fetch::math::NDArray<float>,
                     fetch::math::NDArray<double>, fetch::math::linalg::Matrix<int>,
                     fetch::math::linalg::Matrix<float>, fetch::math::linalg::Matrix<double>>;
TYPED_TEST_CASE(WeightsTest, MyTypes);

TYPED_TEST(WeightsTest, allocation_test)
{
  fetch::ml::ops::Weights<TypeParam> w;
}

TYPED_TEST(WeightsTest, gradient_step_test)
{
  std::shared_ptr<TypeParam> data  = std::make_shared<TypeParam>(8);
  std::shared_ptr<TypeParam> error = std::make_shared<TypeParam>(8);
  std::shared_ptr<TypeParam> gt    = std::make_shared<TypeParam>(8);
  std::vector<int>           dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int>           errorInput({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int>           gtInput({0, 0, 6, -9, -3, 7, -14, -42});
  for (size_t i(0); i < 8; ++i)
  {
    data->Set(i, typename TypeParam::Type(dataInput[i]));
    error->Set(i, typename TypeParam::Type(errorInput[i]));
    gt->Set(i, typename TypeParam::Type(gtInput[i]));
  }

  fetch::ml::ops::Weights<TypeParam> w;
  w.SetData(data);
  ASSERT_TRUE(w.Forward({}) == data);  // Test pointed address, should still be the same
  std::vector<std::shared_ptr<TypeParam>> errorSignal = w.Backward({}, error);
  w.Step();
  ASSERT_TRUE(w.Forward({}) == data);         // Test pointed address, should still be the same
  ASSERT_TRUE(w.Forward({})->AllClose(*gt));  // whit new values
}
