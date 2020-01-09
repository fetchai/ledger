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

#include "core/serializers/main_serializer.hpp"
#include "gtest/gtest.h"
#include "ml/layers/skip_gram.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class SkipGramTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SkipGramTest, math::test::TensorFloatingTypes);

TYPED_TEST(SkipGramTest, saveparams_test)
{
  using DataType  = typename TypeParam::Type;
  using SizeType  = fetch::math::SizeType;
  using LayerType = typename fetch::ml::layers::SkipGram<TypeParam>;
  using SPType    = typename LayerType::SPType;

  SizeType in_size    = 1;
  SizeType out_size   = 1;
  SizeType embed_size = 1;
  SizeType vocab_size = 10;
  SizeType batch_size = 1;

  std::string output_name = "SkipGram_Sigmoid";

  // create input

  TypeParam input({1, batch_size});
  TypeParam context({1, batch_size});
  TypeParam labels({1, batch_size});
  input(0, 0)   = DataType{0};
  context(0, 0) = static_cast<DataType>(5);
  labels(0, 0)  = DataType{0};

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

  // make initial prediction to set internal buffers which must be correctly set in serialisation
  TypeParam prediction0 = layer.Evaluate(output_name, true);

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

  //
  // test that deserialized model gives the same forward prediction as the original layer
  //
  layer.SetInput("SkipGram_Input", input);
  layer.SetInput("SkipGram_Context", context);
  TypeParam prediction = layer.Evaluate(output_name, true);

  // sanity check - serialisation should not affect initial prediction
  ASSERT_TRUE(prediction0.AllClose(prediction, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  layer2.SetInput("SkipGram_Input", input);
  layer2.SetInput("SkipGram_Context", context);
  TypeParam prediction2 = layer2.Evaluate(output_name, true);

  ASSERT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  layer.SetInput(label_name, labels);
  TypeParam loss = layer.Evaluate(error_output);
  layer.BackPropagate(error_output);
  auto grads = layer.GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer.ApplyGradients(grads);

  // train g2
  layer2.SetInput(label_name, labels);
  TypeParam loss2 = layer2.Evaluate(error_output);
  layer2.BackPropagate(error_output);
  auto grads2 = layer2.GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  layer2.ApplyGradients(grads2);

  EXPECT_TRUE(loss.AllClose(loss2, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));

  //
  // test that prediction is different after a back prop and step have been completed
  //

  layer.SetInput("SkipGram_Input", input);      // resets node cache
  layer.SetInput("SkipGram_Context", context);  // resets node cache
  TypeParam prediction3 = layer.Evaluate(output_name);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  //
  // test that the deserialized model gives the same result as the original layer after training
  //

  layer2.SetInput("SkipGram_Input", input);
  layer2.SetInput("SkipGram_Context", context);
  TypeParam prediction5 = layer2.Evaluate(output_name);

  EXPECT_TRUE(prediction3.AllClose(prediction5, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
