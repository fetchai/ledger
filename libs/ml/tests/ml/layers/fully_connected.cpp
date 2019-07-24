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

#include "math/tensor.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "ml/ops/loss_functions.hpp"

#include "gtest/gtest.h"

template <typename T>
class FullyConnectedTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(FullyConnectedTest, MyTypes);

TYPED_TEST(FullyConnectedTest, set_input_and_evaluate_test)  // Use the class as a subgraph
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(100u, 10u);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({10, 10, 2}));
  fc.SetInput("FC_Input", input_data);
  TypeParam output = fc.Evaluate("FC_MatrixMultiply", true);

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, ops_forward_test)  // Use the class as an Ops
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(50, 10);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  TypeParam output(fc.ComputeOutputShape({input_data}));
  fc.Forward({input_data}, output);

  ASSERT_EQ(output.shape().size(), 2);
  ASSERT_EQ(output.shape()[0], 10);
  ASSERT_EQ(output.shape()[1], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, ops_backward_test)  // Use the class as an Ops
{
  fetch::ml::layers::FullyConnected<TypeParam> fc(50, 10);
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));

  TypeParam output(fc.ComputeOutputShape({input_data}));
  fc.Forward({input_data}, output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({10, 2}));

  std::vector<TypeParam> backprop_error = fc.Backward({input_data}, error_signal);
  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 5);
  ASSERT_EQ(backprop_error[0].shape()[1], 10);
  ASSERT_EQ(backprop_error[0].shape()[2], 2);
  // No way to test actual values for now as weights are randomly initialised.
}

TYPED_TEST(FullyConnectedTest, share_weight_with_incompatible_layer)
{
	using ArrayType = TypeParam;
	
	fetch::ml::Graph<ArrayType> g;
	
	std::string input = g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
	std::string fc_1  = g.template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
	 "FC1", {input}, 10u, 20u);
	ASSERT_ANY_THROW(std::string fc_2 = g.template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
	 "FC1", {input}, 10u, 21u));
	ASSERT_ANY_THROW(std::string fc_3 = g.template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
	 "FC1", {input}, 11u, 20u));
}

TYPED_TEST(FullyConnectedTest, share_weight_backward_test)
{
	using ArrayType = TypeParam;
	using DataType = typename ArrayType::Type;
	using GraphType = typename fetch::ml::Graph<ArrayType>;
	
	std::string descriptor = fetch::ml::layers::FullyConnected<ArrayType>::DESCRIPTOR;
	
	// create an auto encoder of two dense layers, both share same weights
	auto g_shared = std::make_shared<GraphType>();
	
	std::string g_shared_input = g_shared->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
	std::string g_shared_intermediate  = g_shared->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
	 "FC1", {g_shared_input}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING, fetch::ml::details::RegularisationType::NONE, static_cast<DataType>(0), fetch::ml::ops::WeightsInitialisation::XAVIER_GLOROT);
	std::string g_shared_output = g_shared->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
	 "FC1", {g_shared_intermediate}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING, fetch::ml::details::RegularisationType::NONE, static_cast<DataType>(0));
	std::string g_shared_label = g_shared->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Label", {});
	std::string g_shared_error = g_shared->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
	 "Error", {g_shared_output, g_shared_label});
	
	// create an auto encoder of two dense layers, both have different weights
	auto g_not_shared = std::make_shared<GraphType>();
	
	std::string g_not_shared_input = g_not_shared->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
	std::string g_not_shared_intermediate  = g_not_shared->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
	 "FC4", {g_not_shared_input}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING, fetch::ml::details::RegularisationType::NONE, static_cast<DataType>(0), fetch::ml::ops::WeightsInitialisation::XAVIER_GLOROT);
	std::string g_not_shared_output = g_not_shared->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
	 "FC5", {g_not_shared_intermediate}, 10u, 10u, fetch::ml::details::ActivationType::NOTHING, fetch::ml::details::RegularisationType::NONE, static_cast<DataType>(0), fetch::ml::ops::WeightsInitialisation::XAVIER_GLOROT);
	std::string g_not_shared_label = g_not_shared->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Label", {});
	std::string g_not_shared_error = g_not_shared->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>(
	 "Error", {g_not_shared_output, g_not_shared_label});
	
	// check that all weights are equal
	std::cout << "**************************BEFORE TRAINING**************************" << std::endl;
	auto g_shared_statedict = g_shared->StateDict();
	std::cout << "state dict for shared weight graph" << std::endl;
	for(auto i : g_shared_statedict.dict_){
		std::cout << "i.first: " << i.first << std::endl;
		std::cout << "i.second.weights_->ToString(): " << std::endl << i.second.weights_->ToString() << std::endl;
	}
	
	auto g_not_shared_statedict = g_not_shared->StateDict();
	std::cout << "state dict for not shared weight graph" << std::endl;
	for(auto i : g_not_shared_statedict.dict_){
		std::cout << "i.first: " << i.first << std::endl;
		std::cout << "i.second.weights_->ToString(): " << std::endl << i.second.weights_->ToString() << std::endl;
	}
	
	// start training
	// set data
	ArrayType data;
	data.Resize({10, 1});
	for(int i=0; i<10; i++){
		data.Set(i, 0, DataType(i));
	}

	// Run 1 iteration of training on g shared
	auto lr = static_cast<DataType>(1);
	fetch::ml::optimisers::SGDOptimiser<ArrayType> g_shared_optimiser(g_shared, {g_shared_input}, g_shared_label, g_shared_error, lr);
	g_shared_optimiser.Run({data}, data, 1);
	// Run 1 iteration of training on g not shared
	fetch::ml::optimisers::SGDOptimiser<ArrayType> g_not_shared_optimiser(g_not_shared, {g_not_shared_input}, g_not_shared_label, g_not_shared_error, lr);
	g_not_shared_optimiser.Run({data}, data, 1);
	
	// check that all weights are equal
	std::cout << "**************************AFTER TRAINING**************************" << std::endl;
	g_shared_statedict = g_shared->StateDict();
	std::cout << "state dict for shared weight graph" << std::endl;
	for(auto i : g_shared_statedict.dict_){
		std::cout << "i.first: " << i.first << std::endl;
		std::cout << "i.second.weights_->ToString(): " << std::endl << i.second.weights_->ToString() << std::endl;
	}
	
	g_not_shared_statedict = g_not_shared->StateDict();
	std::cout << "state dict for not shared weight graph" << std::endl;
	for(auto i : g_not_shared_statedict.dict_){
		std::cout << "i.first: " << i.first << std::endl;
		std::cout << "i.second.weights_->ToString(): " << std::endl << i.second.weights_->ToString() << std::endl;
	}
	
}

TYPED_TEST(FullyConnectedTest, node_forward_test)  // Use the class as a Node
{
  TypeParam data(std::vector<typename TypeParam::SizeType>({5, 10, 2}));
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::FullyConnected<TypeParam>> fc("FullyConnected", 50u,
                                                                              42u);
  fc.AddInput(placeholder);

  TypeParam prediction = fc.Evaluate(true);

  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 42);
  ASSERT_EQ(prediction.shape()[1], 2);
}

TYPED_TEST(FullyConnectedTest, node_backward_test)  // Use the class as a Node
{
  TypeParam data({5, 10, 2});
  std::shared_ptr<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>> placeholder =
      std::make_shared<fetch::ml::Node<TypeParam, fetch::ml::ops::PlaceHolder<TypeParam>>>("Input");
  placeholder->SetData(data);

  fetch::ml::Node<TypeParam, fetch::ml::layers::FullyConnected<TypeParam>> fc("FullyConnected", 50u,
                                                                              42u);
  fc.AddInput(placeholder);
  TypeParam prediction = fc.Evaluate(true);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({42, 2}));
  auto      backprop_error = fc.BackPropagateSignal(error_signal);

  ASSERT_EQ(backprop_error.size(), 1);
  ASSERT_EQ(backprop_error[0].second.shape().size(), 3);
  ASSERT_EQ(backprop_error[0].second.shape()[0], 5);
  ASSERT_EQ(backprop_error[0].second.shape()[1], 10);
  ASSERT_EQ(backprop_error[0].second.shape()[2], 2);
}

TYPED_TEST(FullyConnectedTest, graph_forward_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>("FullyConnected", {"Input"}, 50u,
                                                                   42u);

  TypeParam data({5, 10, 2});
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("FullyConnected", true);
  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 42);
  ASSERT_EQ(prediction.shape()[1], 2);
}

TYPED_TEST(FullyConnectedTest, getStateDict)
{
  using DataType = typename TypeParam::Type;
  using RegType  = fetch::ml::details::RegularisationType;

  fetch::ml::layers::FullyConnected<TypeParam> fc(
      50, 10, fetch::ml::details::ActivationType::NOTHING, RegType::NONE, DataType{0});
  fetch::ml::StateDict<TypeParam> sd = fc.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_EQ(sd.dict_.size(), 2);

  ASSERT_NE(sd.dict_["FCTest_Weights"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["FCTest_Weights"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({10, 50}));

  ASSERT_NE(sd.dict_["FCTest_Bias"].weights_, nullptr);
  EXPECT_EQ(sd.dict_["FCTest_Bias"].weights_->shape(),
            std::vector<typename TypeParam::SizeType>({10, 1}));
}
