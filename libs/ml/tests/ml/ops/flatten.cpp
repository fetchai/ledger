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

#include "ml/ops/flatten.hpp"
#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include <gtest/gtest.h>

template <typename T>
class FlattenTest : public ::testing::Test
{
};

// TODO(private, 520) Adapt for NDArray
using MyTypes =
    ::testing::Types<fetch::math::NDArray<int>, fetch::math::NDArray<float>,
                     fetch::math::NDArray<double>, fetch::math::linalg::Matrix<int>,
                     fetch::math::linalg::Matrix<float>, fetch::math::linalg::Matrix<double>>;
TYPED_TEST_CASE(FlattenTest, MyTypes);

TYPED_TEST(FlattenTest, forward_test)
{
  std::shared_ptr<TypeParam> data = std::make_shared<TypeParam>(std::vector<size_t>({8, 8}));
  fetch::ml::ops::Flatten<TypeParam> op;
  std::shared_ptr<TypeParam>         prediction = op.Forward({data});

  // test correct values
  ASSERT_EQ(prediction->shape().size(), 2);
  ASSERT_EQ(prediction->shape()[0], 1);
  ASSERT_EQ(prediction->shape()[1], 64);
}

TYPED_TEST(FlattenTest, backward_test)
{
  std::shared_ptr<TypeParam> data = std::make_shared<TypeParam>(std::vector<size_t>({8, 8}));
  fetch::ml::ops::Flatten<TypeParam> op;
  std::shared_ptr<TypeParam>         prediction  = op.Forward({data});
  std::shared_ptr<TypeParam>         errorSignal = std::make_shared<TypeParam>(prediction->shape());
  std::vector<std::shared_ptr<TypeParam>> gradients = op.Backward({data}, errorSignal);

  ASSERT_EQ(gradients.size(), 1);
  ASSERT_EQ(gradients[0]->shape().size(), 2);
  ASSERT_EQ(gradients[0]->shape()[0], 8);
  ASSERT_EQ(gradients[0]->shape()[1], 8);
}
