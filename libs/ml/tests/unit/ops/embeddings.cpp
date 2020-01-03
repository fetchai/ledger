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

#include "core/serializers/main_serializer_definition.hpp"
#include "math/base_types.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class EmbeddingsTest : public ::testing::Test
{
};

TYPED_TEST_CASE(EmbeddingsTest, math::test::TensorIntAndFloatingTypes);

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

TYPED_TEST(EmbeddingsTest, saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::Embeddings<TensorType>::SPType;
  using OpType     = fetch::ml::ops::Embeddings<TensorType>;

  TypeParam weights(std::vector<uint64_t>({6, 10}));

  for (uint32_t i(0); i < 10; ++i)
  {
    for (uint32_t j(0); j < 6; ++j)
    {
      weights(j, i) = typename TypeParam::Type(i * 10 + j);
    }
  }
  TypeParam input(std::vector<uint64_t>({2, 1}));
  input.At(0, 0) = typename TypeParam::Type(3);
  input.At(1, 0) = typename TypeParam::Type(5);

  OpType op(6, 10);

  op.SetData(weights);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<TensorType const>(input)}));

  op.Forward({std::make_shared<TensorType const>(input)}, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<TensorType const>(input)}));
  new_op.Forward({std::make_shared<TensorType const>(input)}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(EmbeddingsTest, saveparams_backward)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::Embeddings<TensorType>;
  using SPType     = typename OpType::SPType;

  fetch::ml::ops::Embeddings<TypeParam> op(6, 10);
  TypeParam                             weights(std::vector<uint64_t>({6, 10}));
  for (uint32_t i{0}; i < 10; ++i)
  {
    for (uint32_t j{0}; j < 6; ++j)
    {
      weights(j, i) = static_cast<DataType>(i * 10 + j);
    }
  }

  op.SetData(weights);

  TensorType input(std::vector<uint64_t>({2, 1}));
  input.At(0, 0) = DataType{3};
  input.At(1, 0) = DataType{5};

  TensorType output(op.ComputeOutputShape({std::make_shared<TypeParam>(input)}));
  op.Forward({std::make_shared<TypeParam>(input)}, output);

  TensorType error_signal(std::vector<uint64_t>({6, 2, 1}));
  for (uint32_t j{0}; j < 2; ++j)
  {
    for (uint32_t k{0}; k < 6; ++k)
    {
      error_signal(k, j, 0) = static_cast<DataType>(j * 6 + k);
    }
  }

  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<TypeParam>(input)}, error_signal);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<TypeParam>(input)}, error_signal);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<TypeParam>(input)}, error_signal);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
