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
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class FlattenTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(FlattenTest, MyTypes);

TYPED_TEST(FlattenTest, forward_test)
{
  std::shared_ptr<TypeParam> data =
      std::make_shared<TypeParam>(std::vector<std::uint64_t>({8, 8, 8, 8, 8, 8}));

  {
    fetch::ml::ops::Flatten<TypeParam> op(0u, false);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({1, 262144}));
  }
  {
    fetch::ml::ops::Flatten<TypeParam> op(1u, false);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({8, 32768}));
  }
  {
    fetch::ml::ops::Flatten<TypeParam> op(2u, false);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({8, 8, 4096}));
  }
  {
    fetch::ml::ops::Flatten<TypeParam> op(3u, false);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({8, 8, 8, 512}));
  }
  {
    fetch::ml::ops::Flatten<TypeParam> op(4u, false);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({8, 8, 8, 8, 64}));
  }

  {
    fetch::ml::ops::Flatten<TypeParam> op(0u, true);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({8, 1, 32768}));
  }
  {
    fetch::ml::ops::Flatten<TypeParam> op(1u, true);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({8, 8, 4096}));
  }
  {
    fetch::ml::ops::Flatten<TypeParam> op(2u, true);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({8, 8, 8, 512}));
  }
  {
    fetch::ml::ops::Flatten<TypeParam> op(3u, true);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({8, 8, 8, 8, 64}));
  }
  {
    fetch::ml::ops::Flatten<TypeParam> op(4u, true);
    std::shared_ptr<TypeParam>         prediction = op.Forward({data});
    ASSERT_EQ(prediction->shape(), std::vector<uint64_t>({8, 8, 8, 8, 8, 8}));
  }
}

TYPED_TEST(FlattenTest, backward_test)
{
  std::shared_ptr<TypeParam> data =
      std::make_shared<TypeParam>(std::vector<std::uint64_t>({8, 8, 8, 8, 8, 8}));

  for (unsigned int i(0); i < 5; ++i)
  {
    {
      fetch::ml::ops::Flatten<TypeParam> op(i, false);
      std::shared_ptr<TypeParam>         prediction = op.Forward({data});
      std::shared_ptr<TypeParam> errorSignal = std::make_shared<TypeParam>(prediction->shape());
      std::vector<std::shared_ptr<TypeParam>> gradients = op.Backward({data}, errorSignal);

      ASSERT_EQ(gradients.size(), 1);
      ASSERT_EQ(gradients[0]->shape(), std::vector<std::uint64_t>({8, 8, 8, 8, 8, 8}));
    }
    {
      fetch::ml::ops::Flatten<TypeParam> op(i, true);
      std::shared_ptr<TypeParam>         prediction = op.Forward({data});
      std::shared_ptr<TypeParam> errorSignal = std::make_shared<TypeParam>(prediction->shape());
      std::vector<std::shared_ptr<TypeParam>> gradients = op.Backward({data}, errorSignal);

      ASSERT_EQ(gradients.size(), 1);
      ASSERT_EQ(gradients[0]->shape(), std::vector<std::uint64_t>({8, 8, 8, 8, 8, 8}));
    }
  }
}
