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
  using DataType = typename TypeParam::Type;

  TypeParam data(std::vector<typename TypeParam::SizeType>({12, 25, 4}));

  fetch::math::SizeType in_size    = 10;
  fetch::math::SizeType out_size   = 16;
  fetch::math::SizeType embed_size = 100;
  fetch::math::SizeType vocab_size = 1000;

  auto sg_layer = std::make_shared<fetch::ml::layers::SkipGram<TypeParam>>(in_size, out_size,
                                                                           embed_size, vocab_size);

  sg_layer->SetInput("SkipGram_Input", data);

  TypeParam output = sg_layer->Evaluate("SkipGram_Sigmoid", true);

  // extract saveparams
  auto sp = sg_layer->GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<typename fetch::ml::layers::SkipGram<TypeParam>::SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<typename fetch::ml::layers::SkipGram<TypeParam>::SPType>();
  b >> *dsp2;

  // rebuild
  auto sa2 =
      fetch::ml::utilities::BuildLayer<TypeParam, fetch::ml::layers::SkipGram<TypeParam>>(*dsp2);

  sa2->SetInput("SkipGram_Input", data);
  TypeParam output2 = sa2->Evaluate("SkipGram_Sigmoid", true);

  ASSERT_TRUE(output.AllClose(output2, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}
