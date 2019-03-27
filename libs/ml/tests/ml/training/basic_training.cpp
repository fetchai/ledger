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

#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"

#include "ml/ops/loss_functions/mean_square_error.hpp"
#include "ml/ops/loss_functions/softmax_cross_entropy.hpp"

#include "math/tensor.hpp"
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
typename ArrayType::Type TrainOneBatch(fetch::ml::Graph<ArrayType> &g, ArrayType &data,
                                       ArrayType &gt, std::string &input_name,
                                       std::string &output_name, Criterion &criterion,
                                       typename ArrayType::Type alpha)
{
  typename ArrayType::Type loss = typename ArrayType::Type(0);
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
    for (std::size_t j = 0; j < results.shape().at(1); ++j)
    {
      std::cout << "results.At(j): " << results.At(j) << std::endl;
    }

    auto tmp_loss = criterion.Forward({results, cur_gt});
    std::cout << "tmp_loss: " << tmp_loss << std::endl;

    loss += tmp_loss;
    g.BackPropagate(output_name, criterion.Backward({results, cur_gt}));
  }
  g.Step(alpha);
  return loss;
}

template <typename T>
class FullyConnectedTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(FullyConnectedTest, MyTypes);
//
// TYPED_TEST(FullyConnectedTest, xor_mse_test)  // Use the class as a Node
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

TYPED_TEST(FullyConnectedTest, xor_sce_test)  // Use the class as a Node
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  DataType alpha       = DataType(0.01);
  SizeType input_size  = SizeType(2);
  SizeType output_size = SizeType(2);
  SizeType n_batches   = SizeType(10000);

  fetch::ml::Graph<TypeParam> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  std::string fc1_name   = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {input_name}, input_size, 100u);
  std::string sig_name =
      g.template AddNode<fetch::ml::ops::Sigmoid<TypeParam>>("sigmoid", {fc1_name});
  std::string output_name = g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC2", {sig_name}, 100u, output_size);

  fetch::ml::ops::SoftmaxCrossEntropy<TypeParam> criterion;

  ////////////////////////////////////////
  /// DEFINING DATA AND LABELS FOR XOR ///
  ////////////////////////////////////////

  TypeParam data = GenerateXorData<TypeParam>();
  TypeParam gt   = GenerateXorGt<TypeParam>(output_size);

  g.SetInput(input_name, data);

  /////////////////////////
  /// ONE TRAINING STEP ///
  /////////////////////////

  DataType initial_loss = TrainOneBatch(g, data, gt, input_name, output_name, criterion, alpha);

  /////////////////////
  /// TRAINING LOOP ///
  /////////////////////

  DataType loss;
  for (SizeType epoch{0}; epoch < n_batches; ++epoch)
  {
    loss = TrainOneBatch(g, data, gt, input_name, output_name, criterion, alpha);
    std::cout << "loss: " << loss << std::endl;
  }
  DataType final_loss = loss;

  EXPECT_GT(initial_loss, final_loss);
}
