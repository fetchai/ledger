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

#include "math/free_functions/ml/activation_functions/softmax.hpp"
#include "math/tensor.hpp"
#include "ml/layers/layers.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions.hpp"
#include <gtest/gtest.h>

template <typename ArrayType>
ArrayType GenerateXorData()
{
  ArrayType data{{4, 2}};

  data.Set({0, 0}, typename ArrayType::Type(0));
  data.Set({0, 1}, typename ArrayType::Type(0));

  data.Set({1, 0}, typename ArrayType::Type(0));
  data.Set({1, 1}, typename ArrayType::Type(1));

  data.Set({2, 0}, typename ArrayType::Type(1));
  data.Set({2, 1}, typename ArrayType::Type(0));

  data.Set({3, 0}, typename ArrayType::Type(1));
  data.Set({3, 1}, typename ArrayType::Type(1));

  return data;
}

template <typename ArrayType>
ArrayType GenerateXorGt(typename ArrayType::SizeType dims)
{
  assert((dims == 1) || (dims == 2));

  ArrayType gt{{4, dims}};
  if (dims == 1)
  {
    gt.Set(0, typename ArrayType::Type(0));
    gt.Set(1, typename ArrayType::Type(1));
    gt.Set(2, typename ArrayType::Type(1));
    gt.Set(3, typename ArrayType::Type(0));
  }
  else
  {
    gt.Set({0, 0}, typename ArrayType::Type(1));
    gt.Set({0, 1}, typename ArrayType::Type(0));

    gt.Set({1, 0}, typename ArrayType::Type(0));
    gt.Set({1, 1}, typename ArrayType::Type(1));

    gt.Set({2, 0}, typename ArrayType::Type(0));
    gt.Set({2, 1}, typename ArrayType::Type(1));

    gt.Set({3, 0}, typename ArrayType::Type(1));
    gt.Set({3, 1}, typename ArrayType::Type(0));
  }

  return gt;
}

template <typename ArrayType, typename Criterion>
typename std::pair<typename ArrayType::Type, typename ArrayType::Type> TrainOneBatch(
    fetch::ml::Graph<ArrayType> &g, ArrayType &data, ArrayType &gt, std::string &input_name,
    std::string &output_name, Criterion &criterion, typename ArrayType::Type alpha,
    bool add_softmax = false)
{
  typename ArrayType::Type loss = typename ArrayType::Type(0);
  typename ArrayType::Type acc  = typename ArrayType::Type(0);
  ArrayType                cur_gt{{1, gt.shape().at(1)}};
  ArrayType                cur_input{{1, data.shape().at(1)}};

  for (typename ArrayType::SizeType step{0}; step < 4; ++step)
  {
    cur_input = data.Slice(step);
    g.SetInput(input_name, cur_input, false);

    for (std::size_t i = 0; i < gt.shape().at(1); ++i)
    {
      cur_gt.At(i) = gt.At({step, i});
    }

    auto results = g.Evaluate(output_name);

    for (std::size_t j = 0; j < results.size(); ++j)
    {
      if (add_softmax)
      {
        auto tmp = fetch::math::Softmax(results);
        acc += (gt.At({step, j}) - tmp.At(j)) * (gt.At({step, j}) - tmp.At(j));
      }
      else
      {
        acc += (gt.At({step, j}) - results.At(j)) * (gt.At({step, j}) - results.At(j));
      }
    }

    loss += criterion.Forward({results, cur_gt});

    g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
  }
  g.Step(alpha);

  return std::make_pair(loss, acc);
}

template <typename TypeParam, typename CriterionType, typename ActivationType>
void PlusOneTest()
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  DataType alpha       = DataType(0.01);
  SizeType input_size  = SizeType(1);
  SizeType output_size = SizeType(1);
  SizeType n_batches   = SizeType(100);
  SizeType hidden_size = SizeType(1000);

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
  data.Set({0, 0}, DataType(1));
  data.Set({1, 0}, DataType(2));
  data.Set({2, 0}, DataType(3));
  data.Set({3, 0}, DataType(4));

  TypeParam gt{{4, 1}};
  gt.Set(0, DataType(2));
  gt.Set(1, DataType(3));
  gt.Set(2, DataType(4));
  gt.Set(3, DataType(5));

  g.SetInput(input_name, data);

  /////////////////////////
  /// ONE TRAINING STEP ///
  /////////////////////////

  TypeParam cur_gt{{1, 1}};
  TypeParam cur_input{{1, 1}};
  DataType  loss = DataType(0);

  for (SizeType step{0}; step < 4; ++step)
  {
    cur_input.At(0) = data.At(step);
    g.SetInput(input_name, cur_input);
    cur_gt.At(0) = gt.At(step);

    auto results = g.Evaluate(output_name);
    loss += criterion.Forward({results, cur_gt});

    g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
  }
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
      cur_input.At(0) = data.At(step);
      g.SetInput(input_name, cur_input);
      cur_gt.At(0) = gt.At(step);

      auto results = g.Evaluate(output_name);
      loss += criterion.Forward({results, cur_gt});
      g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
    }

    // This task is so easy the loss should fall on every training step
    EXPECT_GT(current_loss, loss);
    current_loss = loss;
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
  SizeType n_batches   = SizeType(100);
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
  data.Set({0, 0}, DataType(1));
  data.Set({1, 1}, DataType(1));
  data.Set({2, 2}, DataType(1));
  data.Set({3, 3}, DataType(1));

  TypeParam gt{{n_data, SizeType(n_classes.At(0))}};
  gt.Fill(DataType(0));
  gt.Set({0, 1}, DataType(1));
  gt.Set({1, 2}, DataType(1));
  gt.Set({2, 3}, DataType(1));
  gt.Set({3, 0}, DataType(1));

  g.SetInput(input_name, data);

  /////////////////////////
  /// ONE TRAINING STEP ///
  /////////////////////////

  TypeParam cur_gt{{SizeType(1), SizeType(n_classes.At(0))}};
  TypeParam cur_input{{SizeType(1), SizeType(n_classes.At(0))}};
  DataType  loss = DataType(0);

  for (SizeType step{0}; step < n_data; ++step)
  {
    cur_input = data.Slice(step);
    g.SetInput(input_name, cur_input);
    cur_gt       = gt.Slice(step).Unsqueeze();
    auto results = g.Evaluate(output_name);
    loss += criterion.Forward({results, cur_gt, n_classes});
    g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
  }
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
      cur_input = data.Slice(step);
      g.SetInput(input_name, cur_input);
      cur_gt       = gt.Slice(step).Unsqueeze();
      auto results = g.Evaluate(output_name);
      loss += criterion.Forward({results, cur_gt, n_classes});
      g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
    }

    // This task is so easy the loss should fall on every training step
    EXPECT_GT(current_loss, loss);
    current_loss = loss;
    g.Step(alpha);
  }
}

template <typename T>
class BasicTrainingTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(BasicTrainingTest, MyTypes);

TYPED_TEST(BasicTrainingTest, plus_one_test)
{
  // working
  PlusOneTest<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>,
              fetch::ml::ops::Relu<TypeParam>>();

  // currently broken
  // PlusOneTest<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>,
  // fetch::ml::ops::Sigmoid<TypeParam>>();
}

TYPED_TEST(BasicTrainingTest, categorical_plus_one_test)
{
  // working
  CategoricalPlusOneTest<TypeParam, fetch::ml::ops::CrossEntropy<TypeParam>,
                         fetch::ml::ops::Relu<TypeParam>>(true);
  CategoricalPlusOneTest<TypeParam, fetch::ml::ops::SoftmaxCrossEntropy<TypeParam>,
                         fetch::ml::ops::Relu<TypeParam>>(false);

  // broken
  CategoricalPlusOneTest<TypeParam, fetch::ml::ops::CrossEntropy<TypeParam>,
                         fetch::ml::ops::Sigmoid<TypeParam>>(true);
  CategoricalPlusOneTest<TypeParam, fetch::ml::ops::SoftmaxCrossEntropy<TypeParam>,
                         fetch::ml::ops::Sigmoid<TypeParam>>(false);
}

//
// TYPED_TEST(BasicTrainingTest, xor_test)
//{
//  // working
//  XorTest<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>,
//  fetch::ml::ops::Relu<TypeParam>>();
//
//  // broken
////  XorTest<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>,
/// fetch::ml::ops::Sigmoid<TypeParam>>();
//}
//
// TYPED_TEST(BasicTrainingTest, categorical_xor_test)
//{
//  // working
//  CategoricalXorTest<TypeParam, fetch::ml::ops::CrossEntropy<TypeParam>,
//  fetch::ml::ops::Relu<TypeParam>>(true); CategoricalXorTest<TypeParam,
//  fetch::ml::ops::SoftmaxCrossEntropy<TypeParam>, fetch::ml::ops::Relu<TypeParam>>(false);
//
//  // broken
////  CategoricalXorTest<TypeParam, fetch::ml::ops::CrossEntropy<TypeParam>,
/// fetch::ml::ops::Sigmoid<TypeParam>>(true); /  CategoricalXorTest<TypeParam,
/// fetch::ml::ops::SoftmaxCrossEntropy<TypeParam>, fetch::ml::ops::Sigmoid<TypeParam>>(false);
//}

//
// TYPED_TEST(BasicTrainingTest, xor_mse_test)  // Use the class as a Node
//{
//  using SizeType = typename TypeParam::SizeType;
//  using DataType = typename TypeParam::Type;
//
//  DataType alpha = DataType(0.01);
//  SizeType input_size = SizeType(2);
//  SizeType output_size = SizeType(1);
//  SizeType n_batches = SizeType(10000);
//
//  fetch::ml::Graph<TypeParam> g;
//
//  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input",
//  {});
//
//  std::string fc1_name = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>("FC1",
//  {input_name}, input_size, 10u); std::string relu_name = g.template
//  AddNode<fetch::ml::ops::Relu<TypeParam>>("relu", {fc1_name}); std::string fc2_name = g.template
//  AddNode<fetch::ml::layers::FullyConnected<TypeParam>>("FC2", {relu_name}, 10u, 10u); std::string
//  relu2_name = g.template AddNode<fetch::ml::ops::Relu<TypeParam>>("relu2", {fc2_name});
//  std::string output_name = g.template
//  AddNode<fetch::ml::layers::FullyConnected<TypeParam>>("FC3", {relu_name}, 10u, output_size);
//
//  fetch::ml::ops::MeanSquareError<TypeParam> criterion;
//
//  ////////////////////////////////////////
//  /// DEFINING DATA AND LABELS FOR XOR ///
//  ////////////////////////////////////////
//
//  TypeParam data = GenerateXorData<TypeParam>();
//  TypeParam gt = GenerateXorGt<TypeParam>(output_size);
//
//  /////////////////////////
//  /// ONE TRAINING STEP ///
//  /////////////////////////
//
//  DataType initial_loss = TrainOneBatch(g, data, gt, input_name, output_name, criterion, alpha);
//
//  /////////////////////
//  /// TRAINING LOOP ///
//  /////////////////////
//
//  DataType loss;
//  for (SizeType epoch{0}; epoch < n_batches; ++epoch)
//  {
//    loss = TrainOneBatch(g, data, gt, input_name, output_name, criterion, alpha);
//    std::cout << "loss: " << loss << std::endl;
//  }
//  DataType final_loss = loss;
//
//  EXPECT_GT(initial_loss, final_loss);
//}
//
// TYPED_TEST(BasicTrainingTest, xor_sce_test)  // Use the class as a Node
//{
//  using SizeType = typename TypeParam::SizeType;
//  using DataType = typename TypeParam::Type;
//
//  DataType alpha       = DataType(0.01);
//  SizeType input_size  = SizeType(2);
//  SizeType output_size = SizeType(2);
//  SizeType n_batches   = SizeType(100);
//
//  fetch::ml::Graph<TypeParam> g;
//
//  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input",
//  {}); std::string fc1_name   = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
//      "FC1", {input_name}, input_size, 100u);
//  std::string act_name =
//      g.template AddNode<fetch::ml::ops::Relu<TypeParam>>("relu", {fc1_name});
//  std::string output_name = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
//      "FC2", {act_name}, 100u, output_size);
//
//  fetch::ml::ops::SoftmaxCrossEntropy<TypeParam> criterion;
//
//  ////////////////////////////////////////
//  /// DEFINING DATA AND LABELS FOR XOR ///
//  ////////////////////////////////////////
//
//  TypeParam data = GenerateXorData<TypeParam>();
//  TypeParam gt   = GenerateXorGt<TypeParam>(output_size);
//
//  g.SetInput(input_name, data);
//
//  /////////////////////////
//  /// ONE TRAINING STEP ///
//  /////////////////////////
//
//  std::pair<DataType, DataType> initial_loss_acc = TrainOneBatch(g, data, gt, input_name,
//  output_name, criterion, alpha, true); DataType initial_loss = initial_loss_acc.first; DataType
//  initial_acc = initial_loss_acc.second;
//
//  /////////////////////
//  /// TRAINING LOOP ///
//  /////////////////////
//
//  std::pair<DataType, DataType> loss_acc;
//  for (SizeType epoch{0}; epoch < n_batches; ++epoch)
//  {
//    loss_acc = TrainOneBatch(g, data, gt, input_name, output_name, criterion, alpha);
//    std::cout << "loss: " << loss_acc.first << std::endl;
//    std::cout << "acc: " << loss_acc.second<< std::endl;
//    std::cout << "\n" << std::endl;
//  }
//  DataType final_loss = loss_acc.first;
//  DataType final_acc = loss_acc.second;
//
//  EXPECT_GT(initial_loss, final_loss);
//  EXPECT_GT(final_acc, initial_acc);
//}
