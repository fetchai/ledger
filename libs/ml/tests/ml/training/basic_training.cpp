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

#include "math/ml/activation_functions/softmax.hpp"
#include "math/tensor.hpp"
#include "ml/layers/layers.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions.hpp"
#include <gtest/gtest.h>

template <typename ArrayType>
ArrayType GenerateXorData()
{
  ArrayType data{{4, 2}};
  data.Fill(typename ArrayType::Type(0));
  data.Set(1, 1, typename ArrayType::Type(1));
  data.Set(2, 0, typename ArrayType::Type(1));
  data.Set(3, 0, typename ArrayType::Type(1));
  data.Set(3, 1, typename ArrayType::Type(1));

  return data;
}

template <typename ArrayType>
ArrayType GenerateXorGt(typename ArrayType::SizeType dims)
{
  assert((dims == 1) || (dims == 2));

  ArrayType gt{{4, dims}};
  gt.Fill(typename ArrayType::Type(0));
  if (dims == 1)
  {
    gt.Set(1, typename ArrayType::Type(1));
    gt.Set(2, typename ArrayType::Type(1));
  }
  else
  {
    gt.Set(0, 0, typename ArrayType::Type(1));
    gt.Set(1, 1, typename ArrayType::Type(1));
    gt.Set(2, 1, typename ArrayType::Type(1));
    gt.Set(3, 0, typename ArrayType::Type(1));
  }

  return gt;
}

template <typename TypeParam, typename CriterionType, typename ActivationType>
void PlusOneTest()
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  DataType alpha       = DataType(0.01);
  SizeType input_size  = SizeType(1);
  SizeType output_size = SizeType(1);
  SizeType n_batches   = SizeType(300);
  SizeType hidden_size = SizeType(100);

  fetch::ml::Graph<TypeParam> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});
  std::string fc1_name   = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {input_name}, input_size, hidden_size);
  std::string act_name    = g.template AddNode<ActivationType>("", {fc1_name});
  std::string output_name = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {act_name}, hidden_size, output_size);

  CriterionType criterion;

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data{{4, 1}};
  data.Set(0, 0, DataType(1));
  data.Set(1, 0, DataType(2));
  data.Set(2, 0, DataType(3));
  data.Set(3, 0, DataType(4));

  TypeParam gt{{4, 1}};
  gt.Set(0, 0, DataType(2));
  gt.Set(1, 0, DataType(3));
  gt.Set(2, 0, DataType(4));
  gt.Set(3, 0, DataType(5));

  /////////////////////////
  /// ONE TRAINING STEP ///
  /////////////////////////

  TypeParam cur_gt{{1, 1}};
  TypeParam cur_input{{1, 1}};
  DataType  loss = DataType(0);

  for (SizeType step{0}; step < 4; ++step)
  {
    cur_input.At(0, 0) = data.At(step, 0);
    g.SetInput(input_name, cur_input);
    cur_gt.At(0, 0) = gt.At(step, 0);

    auto results = g.Evaluate(output_name);
    loss += criterion.Forward({results, cur_gt});

    g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
  }

  /*
  std::vector<TypeParam> gradients = g.GetGradients();
  for (auto &grad : gradients)
  {
    fetch::math::Multiply(grad, -alpha, grad);
  }
  g.ApplyGradients(gradients);
   */
  g.Step(alpha);

  DataType current_loss = loss;

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = DataType(0);

    for (SizeType step{0}; step < 4; ++step)
    {
      cur_input.At(0, 0) = data.At(step, 0);
      g.SetInput(input_name, cur_input);
      cur_gt.At(0, 0) = gt.At(step, 0);

      auto results = g.Evaluate(output_name);
      loss += criterion.Forward({results, cur_gt});
      g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
    }

    // This task is so easy the loss should fall on every training step
    EXPECT_GE(current_loss, loss);
    current_loss = loss;

    /*std::vector<TypeParam> gradients = g.GetGradients();
    for (auto &grad : gradients)
    {
      fetch::math::Multiply(grad, -alpha, grad);
    }
    g.ApplyGradients(gradients);*/

    g.Step(alpha);


  }
}

template <typename TypeParam, typename CriterionType, typename ActivationType>
void CategoricalPlusOneTest(bool add_softmax = false)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  TypeParam n_classes{1};
  n_classes.At(0) = DataType(4);

  DataType alpha       = DataType(0.01);
  SizeType input_size  = SizeType(n_classes.At(0));
  SizeType output_size = SizeType(n_classes.At(0));
  SizeType n_batches   = SizeType(300);
  SizeType hidden_size = SizeType(100);

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

  CriterionType criterion;

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data{{n_data, SizeType(n_classes.At(0))}};
  data.Fill(DataType(0));
  data.Set(0, 0, DataType(1));
  data.Set(1, 1, DataType(1));
  data.Set(2, 2, DataType(1));
  data.Set(3, 3, DataType(1));

  TypeParam gt{{n_data, SizeType(n_classes.At(0))}};
  gt.Fill(DataType(0));
  gt.Set(0, 1, DataType(1));
  gt.Set(1, 2, DataType(1));
  gt.Set(2, 3, DataType(1));
  gt.Set(3, 0, DataType(1));

  /////////////////////////
  /// ONE TRAINING STEP ///
  /////////////////////////

  DataType loss = DataType(0);

  for (SizeType step{0}; step < n_data; ++step)
  {
    auto cur_input = data.Slice(step).Copy();
    g.SetInput(input_name, cur_input);
    auto cur_gt  = gt.Slice(step).Copy();
    auto results = g.Evaluate(output_name);
    loss += criterion.Forward({results, cur_gt, n_classes});
    g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
  }

  /*
  std::vector<TypeParam> gradients = g.GetGradients();
  for (auto &grad : gradients)
  {
    fetch::math::Multiply(grad, -alpha, grad);
  }
  g.ApplyGradients(gradients);
   */

  g.Step(alpha);

  DataType current_loss = loss;

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = DataType(0);

    for (SizeType step{0}; step < n_data; ++step)
    {
      auto cur_input = data.Slice(step).Copy();
      g.SetInput(input_name, cur_input);
      auto cur_gt  = gt.Slice(step).Copy();
      auto results = g.Evaluate(output_name);
      loss += criterion.Forward({results, cur_gt, n_classes});
      g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
    }

    // This task is so easy the loss should fall on every training step
    EXPECT_GE(current_loss, loss);
    current_loss = loss;

    g.Step(alpha);
    /*
    std::vector<TypeParam> gradients = g.GetGradients();
    for (auto &grad : gradients)
    {
      fetch::math::Multiply(grad, -alpha, grad);
    }
    g.ApplyGradients(gradients);
    */

  }
}

template <typename TypeParam, typename CriterionType, typename ActivationType>
void CategoricalXorTest(bool add_softmax = false)
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  SizeType  n_data{4};
  TypeParam n_classes{1};
  n_classes.At(0) = DataType(2);

  DataType alpha       = DataType(0.01);
  SizeType input_size  = SizeType(n_classes.At(0));
  SizeType output_size = SizeType(n_classes.At(0));
  SizeType n_batches   = SizeType(300);
  SizeType hidden_size = SizeType(100);

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

  CriterionType criterion;

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
  DataType  loss = DataType(0);

  for (SizeType step{0}; step < n_data; ++step)
  {
    cur_input = data.Slice(step).Copy();
    g.SetInput(input_name, cur_input);
    auto results = g.Evaluate(output_name);
    cur_gt       = gt.Slice(step).Copy();
    loss += criterion.Forward({results, cur_gt, n_classes});
    g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
  }

  /*
  std::vector<TypeParam> gradients = g.GetGradients();
  for (auto &grad : gradients)
  {
    fetch::math::Multiply(grad, -alpha, grad);
  }
  g.ApplyGradients(gradients);
   */
  g.Step(alpha);

  DataType current_loss = loss;

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  for (std::size_t i = 0; i < n_batches; ++i)
  {
    loss = DataType(0);

    for (SizeType step{0}; step < n_data; ++step)
    {
      cur_input = data.Slice(step).Copy();
      g.SetInput(input_name, cur_input);
      cur_gt       = gt.Slice(step).Copy();
      auto results = g.Evaluate(output_name);
      loss += criterion.Forward({results, cur_gt, n_classes});
      g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
    }

    EXPECT_GE(current_loss, loss);
    current_loss = loss;

    g.Step(alpha);
    /*
    std::vector<TypeParam> gradients = g.GetGradients();
    for (auto &grad : gradients)
    {
      fetch::math::Multiply(grad, -alpha, grad);
    }
    g.ApplyGradients(gradients);
    */

  }
}

template <typename T>
class BasicTrainingTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(BasicTrainingTest, MyTypes);

TYPED_TEST(BasicTrainingTest, plus_one_relu_test)
{
  PlusOneTest<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>,
              fetch::ml::ops::Relu<TypeParam>>();
}
TYPED_TEST(BasicTrainingTest, plus_one_sigmoid_test)
{
  PlusOneTest<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>,
              fetch::ml::ops::Sigmoid<TypeParam>>();
}

TYPED_TEST(BasicTrainingTest, categorical_plus_one_CE_relu_test)
{
  CategoricalPlusOneTest<TypeParam, fetch::ml::ops::CrossEntropy<TypeParam>,
                         fetch::ml::ops::Relu<TypeParam>>(true);
}
TYPED_TEST(BasicTrainingTest, categorical_plus_one_SCE_relu_test)
{
  CategoricalPlusOneTest<TypeParam, fetch::ml::ops::SoftmaxCrossEntropy<TypeParam>,
                         fetch::ml::ops::Relu<TypeParam>>(false);
}
TYPED_TEST(BasicTrainingTest, categorical_plus_one_CE_sigmoid_test)
{
  CategoricalPlusOneTest<TypeParam, fetch::ml::ops::CrossEntropy<TypeParam>,
                         fetch::ml::ops::Sigmoid<TypeParam>>(true);
}
TYPED_TEST(BasicTrainingTest, categorical_plus_one_SCE_sigmoid_test)
{
  CategoricalPlusOneTest<TypeParam, fetch::ml::ops::SoftmaxCrossEntropy<TypeParam>,
                         fetch::ml::ops::Sigmoid<TypeParam>>(false);
}
TYPED_TEST(BasicTrainingTest, categorical_xor_CE_relu_test)
{
  CategoricalXorTest<TypeParam, fetch::ml::ops::CrossEntropy<TypeParam>,
                     fetch::ml::ops::Relu<TypeParam>>(true);
}
TYPED_TEST(BasicTrainingTest, categorical_xor_SCE_relu_test)
{
  CategoricalXorTest<TypeParam, fetch::ml::ops::SoftmaxCrossEntropy<TypeParam>,
                     fetch::ml::ops::Relu<TypeParam>>(false);
}
