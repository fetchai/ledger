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

#include "ml/ops/embeddings.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class EmbeddingsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(EmbeddingsTest, MyTypes);

TYPED_TEST(EmbeddingsTest, forward_shape)
{
  fetch::ml::ops::Embeddings<TypeParam> e(100, 60);
  std::shared_ptr<TypeParam> input = std::make_shared<TypeParam>(std::vector<uint64_t>({10}));
  for (unsigned int i(0); i < 10; ++i)
  {
    input->At(i) = typename TypeParam::Type(1);
  }
  std::shared_ptr<TypeParam> output = e.Forward({input});

  ASSERT_EQ(output->shape().size(), 2);
  EXPECT_EQ(output->shape()[0], 10);
  EXPECT_EQ(output->shape()[1], 60);
}

TYPED_TEST(EmbeddingsTest, forward)
{
  fetch::ml::ops::Embeddings<TypeParam> e(10, 6);

  std::shared_ptr<TypeParam> weights = std::make_shared<TypeParam>(std::vector<uint64_t>({10, 6}));

  for (unsigned int i(0); i < 10; ++i)
  {
    for (unsigned int j(0); j < 6; ++j)
    {
      weights->Set({i, j}, typename TypeParam::Type(i * 10 + j));
    }
  }

  e.SetData(weights);
  std::shared_ptr<TypeParam> input = std::make_shared<TypeParam>(std::vector<uint64_t>({10}));
  input->Fill(typename TypeParam::Type(0));
  input->At(3)                      = typename TypeParam::Type(1);
  input->At(5)                      = typename TypeParam::Type(1);
  std::shared_ptr<TypeParam> output = e.Forward({input});

  ASSERT_EQ(output->shape().size(), 2);
  EXPECT_EQ(output->shape()[0], 2);
  EXPECT_EQ(output->shape()[1], 6);

  std::vector<int> gt{30, 31, 32, 33, 34, 35, 50, 51, 52, 53, 54, 55};
  for (unsigned int i(0); i < 12; ++i)
  {
    EXPECT_EQ(output->At(i), typename TypeParam::Type(gt[i]));
  }
}

TYPED_TEST(EmbeddingsTest, backward)
{
  fetch::ml::ops::Embeddings<TypeParam> e(10, 6);
  std::shared_ptr<TypeParam> weights = std::make_shared<TypeParam>(std::vector<uint64_t>({10, 6}));
  for (unsigned int i(0); i < 10; ++i)
  {
    for (unsigned int j(0); j < 6; ++j)
    {
      weights->Set({i, j}, typename TypeParam::Type(i * 10 + j));
    }
  }
  e.SetData(weights);
  std::shared_ptr<TypeParam> input = std::make_shared<TypeParam>(std::vector<uint64_t>({10}));
  input->Fill(typename TypeParam::Type(0));
  input->At(3)                      = typename TypeParam::Type(1);
  input->At(5)                      = typename TypeParam::Type(1);
  std::shared_ptr<TypeParam> output = e.Forward({input});

  std::shared_ptr<TypeParam> errorSignal =
      std::make_shared<TypeParam>(std::vector<uint64_t>({2, 6}));

  for (unsigned int j(0); j < 6; ++j)
  {
    errorSignal->Set({0, j}, typename TypeParam::Type(j));
  }
  for (unsigned int j(0); j < 6; ++j)
  {
    errorSignal->Set({1, j}, typename TypeParam::Type(j + 6));
  }
  e.Backward({input}, errorSignal);
  e.Step(typename TypeParam::Type(1));

  output = e.Forward({input});
  std::vector<int> gt{30, 30, 30, 30, 30, 30, 44, 44, 44, 44, 44, 44};

  for (unsigned int j(0); j < 12; ++j)
  {
    EXPECT_EQ(output->At(j), typename TypeParam::Type(gt[j]));
  }
}
