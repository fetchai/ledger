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

#include "test_types.hpp"

#include "math/activation_functions/softmax.hpp"
#include "math/statistics/mean.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/loss_functions/softmax_cross_entropy_loss.hpp"
#include "ml/ops/placeholder.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {
namespace basic_training_details {
template <typename TensorType>
TensorType GenerateXorData()
{
  TensorType data{{2, 4}};
  data.Fill(typename TensorType::Type(0));
  data.Set(1, 1, typename TensorType::Type(1));
  data.Set(0, 2, typename TensorType::Type(1));
  data.Set(0, 3, typename TensorType::Type(1));
  data.Set(1, 3, typename TensorType::Type(1));

  return data;
}

template <typename TensorType>
TensorType GenerateXorGt(typename TensorType::SizeType dims)
{
  assert((dims == 1) || (dims == 2));

  TensorType gt{{dims, 4}};
  gt.Fill(typename TensorType::Type(0));
  if (dims == 1)
  {
    gt.Set(1, typename TensorType::Type(1));
    gt.Set(2, typename TensorType::Type(1));
  }
  else
  {
    gt.Set(0, 0, typename TensorType::Type(1));
    gt.Set(1, 1, typename TensorType::Type(1));
    gt.Set(1, 2, typename TensorType::Type(1));
    gt.Set(0, 3, typename TensorType::Type(1));
  }

  return gt;
}

template <typename TypeParam, typename CriterionType, typename ActivationType>
void PlusOneTest()
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  auto alpha       = fetch::math::Type<DataType>("0.005");
  auto input_size  = SizeType(1);
  auto output_size = SizeType(1);
  auto n_batches   = SizeType(300);
  auto hidden_size = SizeType(100);

  fetch::ml::Graph<TypeParam> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});
  std::string fc1_name   = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {input_name}, input_size, hidden_size);
  std::string act_name    = g.template AddNode<ActivationType>("", {fc1_name});
  std::string output_name = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {act_name}, hidden_size, output_size);

  std::string label_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string error_name = g.template AddNode<CriterionType>("Error", {output_name, label_name});
  g.Compile();

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data{{4, 1}};
  data.Set(0, 0, DataType{1});
  data.Set(1, 0, DataType{2});
  data.Set(2, 0, DataType{3});
  data.Set(3, 0, DataType{4});

  TypeParam gt{{4, 1}};
  gt.Set(0, 0, DataType{2});
  gt.Set(1, 0, DataType{3});
  gt.Set(2, 0, DataType{4});
  gt.Set(3, 0, DataType{5});

  /////////////////////////
  /// ONE TRAINING STEP ///
  /////////////////////////

  TypeParam cur_gt{{1, 1}};
  TypeParam cur_input{{1, 1}};
  auto      loss = DataType{0};

  for (SizeType step{0}; step < 4; ++step)
  {
    cur_input.At(0, 0) = data.At(step, 0);
    g.SetInput(input_name, cur_input);
    cur_gt.At(0, 0) = gt.At(step, 0);
    g.SetInput(label_name, cur_gt);

    auto error_tensor = g.Evaluate(error_name);
    loss += error_tensor(0, 0);
    g.BackPropagate(error_name);
  }

  for (auto &w : g.GetTrainables())
  {
    TypeParam grad = w->GetGradientsReferences();
    fetch::math::Multiply(grad, DataType{-alpha}, grad);
    w->ApplyGradient(grad);
  }

  DataType current_loss = loss;

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = DataType{0};

    for (SizeType step{0}; step < 4; ++step)
    {
      cur_input.At(0, 0) = data.At(step, 0);
      g.SetInput(input_name, cur_input);
      cur_gt.At(0, 0) = gt.At(step, 0);
      g.SetInput(label_name, cur_gt);

      auto error_tensor = g.Evaluate(error_name);
      loss += error_tensor(0, 0);
      g.BackPropagate(error_name);
    }

    // This task is so easy the loss should fall on every training step
    EXPECT_GE(current_loss, loss);
    current_loss = loss;

    for (auto &w : g.GetTrainables())
    {
      TypeParam grad = w->GetGradientsReferences();
      fetch::math::Multiply(grad, DataType{-alpha}, grad);
      w->ApplyGradient(grad);
    }
  }
}

template <typename TypeParam, typename CriterionType, typename ActivationType>
void CategoricalPlusOneTest(bool add_softmax = false)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  TypeParam n_classes{1};
  n_classes.At(0) = DataType(4);

  auto alpha       = fetch::math::Type<DataType>("0.01");
  auto input_size  = SizeType(n_classes.At(0));
  auto output_size = SizeType(n_classes.At(0));
  auto n_batches   = SizeType(300);
  auto hidden_size = SizeType(100);

  fetch::ml::Graph<TypeParam> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});
  std::string fc1_name   = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "", {input_name}, input_size, hidden_size);
  std::string act_name    = g.template AddNode<ActivationType>("", {fc1_name});
  std::string output_name = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "", {act_name}, hidden_size, output_size);
  if (add_softmax)
  {
    output_name = g.template AddNode<fetch::ml::ops::Softmax<TypeParam>>("", {output_name});
  }

  std::string label_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string error_name = g.template AddNode<CriterionType>("Error", {output_name, label_name});
  g.Compile();

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data{{n_data, SizeType(n_classes.At(0))}};
  data.Fill(DataType{0});
  data.Set(0, 0, DataType{1});
  data.Set(1, 1, DataType{1});
  data.Set(2, 2, DataType{1});
  data.Set(3, 3, DataType{1});

  TypeParam gt{{n_data, SizeType(n_classes.At(0))}};
  gt.Fill(DataType{0});
  gt.Set(0, 1, DataType{1});
  gt.Set(1, 2, DataType{1});
  gt.Set(2, 3, DataType{1});
  gt.Set(3, 0, DataType{1});

  /////////////////////////
  /// ONE TRAINING STEP ///
  /////////////////////////

  auto loss = DataType{0};

  for (SizeType step{0}; step < n_data; ++step)
  {
    auto cur_input = data.View(step).Copy();
    g.SetInput(input_name, cur_input);
    auto cur_gt = gt.View(step).Copy();
    g.SetInput(label_name, cur_gt);

    auto error_tensor = g.Evaluate(error_name);
    loss += error_tensor(0, 0);
    g.BackPropagate(error_name);
  }

  for (auto &w : g.GetTrainables())
  {
    TypeParam grad = w->GetGradientsReferences();
    fetch::math::Multiply(grad, DataType{-alpha}, grad);
    w->ApplyGradient(grad);
  }

  DataType current_loss = loss;

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = DataType{0};

    for (SizeType step{0}; step < n_data; ++step)
    {
      auto cur_input = data.View(step).Copy();
      g.SetInput(input_name, cur_input);
      auto cur_gt = gt.View(step).Copy();
      g.SetInput(label_name, cur_gt);

      auto error_tensor = g.Evaluate(error_name);
      loss += error_tensor(0, 0);
      g.BackPropagate(error_name);
    }

    // This task is so easy the loss should fall on every training step
    EXPECT_GE(current_loss, loss);
    current_loss = loss;

    for (auto &w : g.GetTrainables())
    {
      TypeParam grad = w->GetGradientsReferences();
      fetch::math::Multiply(grad, DataType{-alpha}, grad);
      w->ApplyGradient(grad);
    }
  }
}

template <typename TypeParam, typename CriterionType, typename ActivationType>
void CategoricalXorTest(bool add_softmax = false)
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  TypeParam n_classes{1};
  n_classes.At(0) = DataType(2);

  auto alpha       = fetch::math::Type<DataType>("0.01");
  auto input_size  = SizeType(n_classes.At(0));
  auto output_size = SizeType(n_classes.At(0));
  auto n_batches   = SizeType(300);
  auto hidden_size = SizeType(100);

  fetch::ml::Graph<TypeParam> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});
  std::string fc1_name   = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "", {input_name}, input_size, hidden_size);
  std::string act_name    = g.template AddNode<ActivationType>("", {fc1_name});
  std::string output_name = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "", {act_name}, hidden_size, output_size);
  if (add_softmax)
  {
    output_name = g.template AddNode<fetch::ml::ops::Softmax<TypeParam>>("", {output_name});
  }

  std::string label_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});

  std::string error_name = g.template AddNode<CriterionType>("Error", {output_name, label_name});
  g.Compile();

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data = GenerateXorData<TypeParam>();
  TypeParam gt   = GenerateXorGt<TypeParam>(SizeType(n_classes.At(0)));

  /////////////////////////
  /// ONE TRAINING STEP ///
  /////////////////////////

  TypeParam cur_gt{{SizeType(1), SizeType(n_classes.At(0))}};
  TypeParam cur_input{{SizeType(1), SizeType(n_classes.At(0))}};
  auto      loss = DataType{0};

  for (SizeType step{0}; step < n_data; ++step)
  {
    cur_input = data.View(step).Copy();
    g.SetInput(input_name, cur_input);
    cur_gt = gt.View(step).Copy();
    g.SetInput(label_name, cur_gt);

    auto error_tensor = g.Evaluate(error_name);
    loss += error_tensor(0, 0);
    g.BackPropagate(error_name);
  }

  for (auto &w : g.GetTrainables())
  {
    TypeParam grad = w->GetGradientsReferences();
    fetch::math::Multiply(grad, DataType{-alpha}, grad);
    w->ApplyGradient(grad);
  }

  DataType current_loss = loss;

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = DataType{0};

    for (SizeType step{0}; step < n_data; ++step)
    {
      cur_input = data.View(step).Copy();
      g.SetInput(input_name, cur_input);
      cur_gt            = gt.View(step).Copy();
      auto error_tensor = g.Evaluate(error_name);
      loss += error_tensor(0, 0);
      g.BackPropagate(error_name);
    }

    EXPECT_GE(current_loss, loss);
    current_loss = loss;

    for (auto &w : g.GetTrainables())
    {
      TypeParam grad = w->GetGradientsReferences();
      fetch::math::Multiply(grad, DataType{-alpha}, grad);
      w->ApplyGradient(grad);
    }
  }
}

}  // namespace basic_training_details

template <typename T>
class BasicTrainingTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(BasicTrainingTest, math::test::HighPrecisionTensorFloatingTypes, );

TYPED_TEST(BasicTrainingTest, plus_one_relu_test)
{
  basic_training_details::PlusOneTest<TypeParam, fetch::ml::ops::MeanSquareErrorLoss<TypeParam>,
                                      fetch::ml::ops::Relu<TypeParam>>();
}
TYPED_TEST(BasicTrainingTest, plus_one_sigmoid_test)
{
  basic_training_details::PlusOneTest<TypeParam, fetch::ml::ops::MeanSquareErrorLoss<TypeParam>,
                                      fetch::ml::ops::Sigmoid<TypeParam>>();
}

TYPED_TEST(BasicTrainingTest, categorical_plus_one_CE_relu_test)
{
  basic_training_details::CategoricalPlusOneTest<
      TypeParam, fetch::ml::ops::CrossEntropyLoss<TypeParam>, fetch::ml::ops::Relu<TypeParam>>(
      true);
}
TYPED_TEST(BasicTrainingTest, categorical_plus_one_SCE_relu_test)
{
  basic_training_details::CategoricalPlusOneTest<TypeParam,
                                                 fetch::ml::ops::SoftmaxCrossEntropyLoss<TypeParam>,
                                                 fetch::ml::ops::Relu<TypeParam>>(false);
}
TYPED_TEST(BasicTrainingTest, categorical_plus_one_CE_sigmoid_test)
{
  basic_training_details::CategoricalPlusOneTest<
      TypeParam, fetch::ml::ops::CrossEntropyLoss<TypeParam>, fetch::ml::ops::Sigmoid<TypeParam>>(
      true);
}
TYPED_TEST(BasicTrainingTest, categorical_plus_one_SCE_sigmoid_test)
{
  basic_training_details::CategoricalPlusOneTest<TypeParam,
                                                 fetch::ml::ops::SoftmaxCrossEntropyLoss<TypeParam>,
                                                 fetch::ml::ops::Sigmoid<TypeParam>>(false);
}
TYPED_TEST(BasicTrainingTest, categorical_xor_CE_relu_test)
{
  basic_training_details::CategoricalXorTest<TypeParam, fetch::ml::ops::CrossEntropyLoss<TypeParam>,
                                             fetch::ml::ops::Relu<TypeParam>>(true);
}
TYPED_TEST(BasicTrainingTest, categorical_xor_SCE_relu_test)
{
  basic_training_details::CategoricalXorTest<TypeParam,
                                             fetch::ml::ops::SoftmaxCrossEntropyLoss<TypeParam>,
                                             fetch::ml::ops::Relu<TypeParam>>(false);
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
