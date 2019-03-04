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

#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include "ml/ops/activation.hpp"
#include <gtest/gtest.h>

template <typename T>
class SoftmaxTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(SoftmaxTest, MyTypes);

TYPED_TEST(SoftmaxTest, forward_test)
{
  std::shared_ptr<TypeParam> data = std::make_shared<TypeParam>(8);
  std::shared_ptr<TypeParam> gt   = std::make_shared<TypeParam>(8);
  std::vector<double>        dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double>        gtInput({2.1437e-03, 1.0673e-04, 1.5840e-02, 1.4444e-05, 1.1704e-01,
                               1.9548e-06, 8.6485e-01, 2.6456e-07});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    data->Set(i, typename TypeParam::Type(dataInput[i]));
    gt->Set(i, typename TypeParam::Type(gtInput[i]));
  }
  fetch::ml::ops::Softmax<TypeParam> op;
  std::shared_ptr<TypeParam>         prediction = op.Forward({data});

  // test correct values
  ASSERT_TRUE(
      prediction->AllClose(*gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}

TYPED_TEST(SoftmaxTest, backward_test)
{
  std::shared_ptr<TypeParam> data  = std::make_shared<TypeParam>(8);
  std::shared_ptr<TypeParam> error = std::make_shared<TypeParam>(8);
  std::shared_ptr<TypeParam> gt    = std::make_shared<TypeParam>(8);
  std::vector<double>        dataInput({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double>        errorInput({0, 0, 0, 0, 1, 0, 0, 0});
  std::vector<double>        gtInput({-0.0002509, -1.2492e-05, -0.0018540, -1.6906e-06, 1.0335e-01,
                               -2.2880e-07, -1.0123e-01, -3.0965e-08});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    data->Set(i, typename TypeParam::Type(dataInput[i]));
    error->Set(i, typename TypeParam::Type(errorInput[i]));
    gt->Set(i, typename TypeParam::Type(gtInput[i]));
  }
  fetch::ml::ops::Softmax<TypeParam>      op;
  std::vector<std::shared_ptr<TypeParam>> prediction = op.Backward({data}, error);

  for (std::size_t j = 0; j < prediction[0]->size(); ++j)
  {
    std::cout << "prediction[0]->At(j): " << prediction[0]->At(j) << std::endl;
  }

  // test correct values
  ASSERT_TRUE(
      prediction[0]->AllClose(*gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}
