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

#include "core/serializers/main_serializer.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"
#include "ml/serializers/ml_types.hpp"

template <typename T>
class SkipGramTest : public ::testing::Test
{
};
using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(SkipGramTest, MyTypes);

TYPED_TEST(SkipGramTest, saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = typename TypeParam::SizeType;
  using LayerType = typename fetch::ml::layers::SkipGram<TypeParam>;
  using SPType    = typename LayerType::SPType;

  SizeType in_size    = 1;
  SizeType out_size   = 1;
  SizeType embed_size = 10;
  SizeType vocab_size = 100;

  std::string output_name = "SkipGram_Sigmoid";

  // create input

  TypeParam input({1, 1});
  TypeParam context({1, 1});
  input.FillUniformRandom();
  context.FillUniformRandom();

  // create labels
  TypeParam labels({1, 1});
  labels.FillUniformRandom();

  // Create layer
  LayerType layer(in_size, out_size, embed_size, vocab_size);

  // add label node
  std::string label_name =
      layer.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});

  // Add loss function
  std::string error_output = layer.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
      "num_error", {output_name, label_name});

  // set input and ForwardPropagate
  layer.SetInput("SkipGram_Input", input);
  layer.SetInput("SkipGram_Context", context);
  TypeParam prediction = layer.ForwardPropagate(output_name, true);

  // extract saveparams
  auto sp = layer.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild
  auto layer2 = *(fetch::ml::utilities::BuildLayer<TypeParam, LayerType>(dsp2));

  // test equality
  layer.SetInput("SkipGram_Input", input);
  layer.SetInput("SkipGram_Context", context);
  prediction = layer.ForwardPropagate(output_name, true);
  layer2.SetInput("SkipGram_Input", input);
  layer2.SetInput("SkipGram_Context", context);
  TypeParam prediction2 = layer2.ForwardPropagate(output_name, true);

  ASSERT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  layer.SetInput(label_name, labels);
  TypeParam loss = layer.ForwardPropagate(error_output);
  layer.BackPropagateError(error_output);
  layer.Step(DataType{0.1f});

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.ForwardPropagate(error_output);
  layer2.BackPropagateError(error_output);
  layer2.Step(DataType{0.1f});

  EXPECT_TRUE(loss.AllClose(loss2, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));

  // new random input
  input.FillUniformRandom();
  context.FillUniformRandom();

  layer.SetInput("SkipGram_Input", input);
  layer.SetInput("SkipGram_Context", context);
  TypeParam prediction3 = layer.ForwardPropagate(output_name);

  layer2.SetInput("SkipGram_Input", input);
  layer2.SetInput("SkipGram_Context", context);
  TypeParam prediction4 = layer2.ForwardPropagate(output_name);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(prediction3.AllClose(prediction4, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}
