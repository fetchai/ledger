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
#include "ml/ops/embeddings.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace {

template <typename T>
class EmbeddingsTest : public ::testing::Test
{
};

TYPED_TEST_CASE(EmbeddingsTest, fetch::math::test::TensorIntAndFloatingTypes);

TYPED_TEST(EmbeddingsTest, forward_shape)
{
  using SizeType = fetch::math::SizeType;
  fetch::ml::ops::Embeddings<TypeParam> e(SizeType(60), SizeType(100));
  TypeParam                             input(std::vector<uint64_t>({10, 1}));
  for (uint32_t i(0); i < 10; ++i)
  {
    input.At(i, 0) = typename TypeParam::Type(i);
  }

  TypeParam output(e.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  e.Forward({std::make_shared<TypeParam>(input)}, output);

  ASSERT_EQ(output.shape(), std::vector<typename TypeParam::SizeType>({60, 10, 1}));
}

TYPED_TEST(EmbeddingsTest, forward)
{
  fetch::ml::ops::Embeddings<TypeParam> e(6, 10);

  TypeParam weights(std::vector<uint64_t>({6, 10}));

  for (uint32_t i(0); i < 10; ++i)
  {
    for (uint32_t j(0); j < 6; ++j)
    {
      weights(j, i) = typename TypeParam::Type(i * 10 + j);
    }
  }

  e.SetData(weights);

  TypeParam input(std::vector<uint64_t>({2, 1}));
  input.At(0, 0) = typename TypeParam::Type(3);
  input.At(1, 0) = typename TypeParam::Type(5);

  TypeParam output(e.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  e.Forward({std::make_shared<TypeParam>(input)}, output);

  ASSERT_EQ(output.shape(), std::vector<typename TypeParam::SizeType>({6, 2, 1}));

  std::vector<int> gt{30, 31, 32, 33, 34, 35, 50, 51, 52, 53, 54, 55};

  for (uint32_t i{0}; i < 2; ++i)
  {
    for (uint32_t j{0}; j < 6; ++j)
    {
      EXPECT_EQ(output.At(j, i, 0), typename TypeParam::Type(gt[(i * 6) + j]));
    }
  }
}

TYPED_TEST(EmbeddingsTest, backward)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SizeType   = fetch::math::SizeType;

  fetch::ml::ops::Embeddings<TypeParam> e(6, 10);
  TypeParam                             weights(std::vector<uint64_t>({6, 10}));
  for (SizeType i{0}; i < 10; ++i)
  {
    for (SizeType j{0}; j < 6; ++j)
    {
      weights(j, i) = static_cast<DataType>(i * 10 + j);
    }
  }

  e.SetData(weights);

  TensorType input(std::vector<uint64_t>({2, 1}));
  input.At(0, 0) = DataType{3};
  input.At(1, 0) = DataType{5};

  TensorType output(e.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  e.Forward({std::make_shared<TypeParam>(input)}, output);

  TensorType error_signal(std::vector<uint64_t>({6, 2, 1}));
  for (SizeType j{0}; j < 2; ++j)
  {
    for (SizeType k{0}; k < 6; ++k)
    {
      error_signal(k, j, 0) = static_cast<DataType>((j * 6) + k);
    }
  }

  e.Backward({std::make_shared<TypeParam>(input)}, error_signal);

  TensorType grad = e.GetGradientsReferences();
  fetch::math::Multiply(grad, DataType{-1}, grad);
  e.ApplyGradient(grad);

  // Get a copy of the gradients and check that they were zeroed out after Step
  TensorType grads_copy = e.GetGradientsReferences();

  EXPECT_TRUE(TensorType::Zeroes({6, 1}).AllClose(grads_copy.View(SizeType(input(0, 0))).Copy()));
  EXPECT_TRUE(TensorType::Zeroes({6, 1}).AllClose(grads_copy.View(SizeType(input(1, 0))).Copy()));

  output = TensorType(e.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  e.Forward({std::make_shared<TypeParam>(input)}, output);

  std::vector<int> gt{30, 30, 30, 30, 30, 30, 44, 44, 44, 44, 44, 44};

  for (SizeType j{0}; j < 2; ++j)
  {
    for (SizeType k{0}; k < 6; ++k)
    {
      EXPECT_EQ(output.At(k, j, 0), static_cast<DataType>(gt[(j * 6) + k]));
    }
  }
}

}  // namespace
