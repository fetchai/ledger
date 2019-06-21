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

#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/estimators/dnn_classifier.hpp"

#include <gtest/gtest.h>

template <typename T>
class EstimatorsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(EstimatorsTest, MyTypes);

template <typename TypeParam>
void PrepareTestDataAndLabels1D(TypeParam &data, TypeParam &gt, TypeParam &test_datum)
{
  data       = TypeParam::FromString("0, 1, 0; 1, 0, 0; 0, 0, 1");
  gt         = TypeParam::FromString("0, 1, 0; 1, 0, 0; 0, 0, 1");
  test_datum = TypeParam::FromString("0; 1; 0");
}

// template <typename TypeParam>
// void PrepareTestDataAndLabels2D(TypeParam &data, TypeParam &gt)
//{
//  using DataType = typename TypeParam::Type;
//
//  data.Resize({2, 2, 3});
//  data.Set(0, 0, 0, DataType(1));
//  data.Set(0, 1, 0, DataType(2));
//  data.Set(1, 0, 0, DataType(3));
//  data.Set(1, 1, 0, DataType(4));
//
//  data.Set(0, 0, 1, DataType(5));
//  data.Set(0, 1, 1, DataType(6));
//  data.Set(1, 0, 1, DataType(7));
//  data.Set(1, 1, 1, DataType(8));
//
//  data.Set(0, 0, 2, DataType(9));
//  data.Set(0, 1, 2, DataType(10));
//  data.Set(1, 0, 2, DataType(11));
//  data.Set(1, 1, 2, DataType(12));
//
//  gt.Resize({2, 3});
//  gt.Set(0, 0, DataType(2));
//  gt.Set(1, 0, DataType(3));
//
//  gt.Set(0, 1, DataType(6));
//  gt.Set(1, 1, DataType(7));
//
//  gt.Set(0, 2, DataType(10));
//  gt.Set(1, 2, DataType(11));
//}

template <typename TypeParam, typename DataType, typename EstimatorType>
EstimatorType SetupEstimator(fetch::ml::estimator::EstimatorConfig<DataType> &estimator_config,
                             TypeParam &data, TypeParam &gt)
{
  // setup dataloader
  using DataLoaderType = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>;
  auto data_loader_ptr = std::make_shared<DataLoaderType>();
  data_loader_ptr->AddData(data, gt);

  // run estimator in training mode
  return EstimatorType(estimator_config, data_loader_ptr, {3, 100, 100, 3});
}

TYPED_TEST(EstimatorsTest, sgd_dnnclasifier)
{

  using DataType      = typename TypeParam::Type;
  using EstimatorType = fetch::ml::estimator::DNNClassifier<TypeParam>;

  fetch::math::SizeType n_training_steps = 10;

  fetch::ml::estimator::EstimatorConfig<DataType> estimator_config;
  estimator_config.learning_rate = static_cast<DataType>(0.1);
  estimator_config.lr_decay      = static_cast<DataType>(0.99);
  estimator_config.opt           = fetch::ml::optimisers::OptimiserType::SGD;

  // set up data
  TypeParam data, gt, test_datum;
  PrepareTestDataAndLabels1D<TypeParam>(data, gt, test_datum);

  // set up estimator
  EstimatorType estimator =
      SetupEstimator<TypeParam, DataType, EstimatorType>(estimator_config, data, gt);

  // test loss decreases
  fetch::math::SizeType count{0};
  while (count < n_training_steps)
  {
    DataType loss{0};
    DataType later_loss{0};
    ASSERT_TRUE(estimator.Train(1, loss));
    ASSERT_TRUE(estimator.Train(1, later_loss));
    //    EXPECT_LT(later_loss, loss);
    count++;
  }

  ASSERT_TRUE(estimator.Train(100));

  // test prediction performance
  TypeParam pred({3, 1});

  estimator.Train(100);
  bool success = estimator.Predict(test_datum, pred);

  ASSERT_TRUE(success);
  EXPECT_TRUE(pred.AllClose(test_datum, static_cast<DataType>(1e-2), static_cast<DataType>(1e-2)));
}

//
//
//
//
// template <typename TypeParam>
// std::shared_ptr<fetch::ml::Graph<TypeParam>> PrepareTestGraph(
//    typename TypeParam::SizeType input_size, typename TypeParam::SizeType output_size,
//    std::string &input_name, std::string &output_name)
//{
//  using SizeType = typename TypeParam::SizeType;
//
//  SizeType hidden_size = SizeType(10);
//
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g(std::make_shared<fetch::ml::Graph<TypeParam>>());
//
//  input_name = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("", {});
//
//  std::string fc1_name = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
//      "FC1", {input_name}, input_size, hidden_size);
//  std::string act_name = g->template AddNode<fetch::ml::ops::Relu<TypeParam>>("", {fc1_name});
//  output_name          = g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
//      "FC2", {act_name}, hidden_size, output_size);
//
//  return g;
//}
//
//
// TYPED_TEST(EstimatorsTest, sgd_optimiser_training)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.1f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(1, 1, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels1D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::SGDOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do 2 optimiser steps
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 1.83612, 1e-5);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.01965, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.18362, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.08435, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.01474, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, sgd_optimiser_training_2D)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.01f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(4, 2, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels2D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::SGDOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do 2 optimiser steps
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 117.98718, 1e-4);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), -0.02427, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.48276, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), -0.02699, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.45438, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, momentum_optimiser_training)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.04f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(1, 1, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels1D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::MomentumOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do 2 optimiser steps to ensure that momentum was applied
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 1.11945, 1e-5);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.05633, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.18362, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.14914, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.01474, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, momentum_optimiser_training_2D)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.01f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(4, 2, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels2D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::MomentumOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do 2 optimiser steps
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 117.98717, 1e-4);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), -0.00685, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.28445, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.02250, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.08207, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, adagrad_optimiser_training)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.04f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(1, 1, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels1D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::AdaGradOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do multiple steps
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 2.04488, 1e-5);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.06323, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.18362, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.06163, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.01474, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, adagrad_optimiser_training_2D)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.04f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(4, 2, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels2D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::AdaGradOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do multiple steps
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 13.57873, 1e-5);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.062189, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.102255, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.061548, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.111611, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, rmsprop_optimiser_training)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.01f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(1, 1, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels1D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::RMSPropOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do multiple steps
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 2.58567, 1e-5);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.05176, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.18362, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.05076, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.01474, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, rmsprop_optimiser_training_2D)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.01f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(4, 2, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels2D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::RMSPropOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do multiple steps
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 18.19288, 1e-5);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.05188, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.11242, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.05076, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.12341, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, adam_optimiser_training)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.01f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(1, 1, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels1D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::AdamOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do multiple steps
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 4.21160, 1e-5);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.02162, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.18362, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.02160, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.01474, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, adam_optimiser_training_2D)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.01f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//      PrepareTestGraph<TypeParam>(4, 2, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels2D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::AdamOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do multiple steps
//  optimiser.Run({data}, gt);
//  DataType loss = optimiser.Run({data}, gt);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 32.87280, 1e-4);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.02160, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.14116, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.02161, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.15418, 1e-5);
//}
//
// TYPED_TEST(EstimatorsTest, adam_optimiser_minibatch_training)
//{
//  using DataType = typename TypeParam::Type;
//
//  DataType learning_rate = DataType{0.01f};
//
//  // Prepare model
//  std::string                                  input_name;
//  std::string                                  output_name;
//  std::shared_ptr<fetch::ml::Graph<TypeParam>> g =
//
//      PrepareTestGraph<TypeParam>(1, 1, input_name, output_name);
//
//  // Prepare data and labels
//  TypeParam data;
//  TypeParam gt;
//  PrepareTestDataAndLabels1D(data, gt);
//
//  // Initialize Optimiser
//  fetch::ml::optimisers::AdamOptimiser<TypeParam, fetch::ml::ops::MeanSquareError<TypeParam>>
//      optimiser(g, {input_name}, output_name, learning_rate);
//
//  // Do multiple steps
//  optimiser.Run({data}, gt, 3);
//  DataType loss = optimiser.Run({data}, gt, 2);
//
//  // Test loss
//  EXPECT_NEAR(static_cast<double>(loss), 2.60907, 1e-5);
//
//  // Test weights
//  std::vector<TypeParam> weights = g->get_weights();
//  EXPECT_NEAR(static_cast<double>(weights[0].At(9, 0)), 0.04839, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[1].At(4, 0)), -0.18362, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[2].At(0, 0)), 0.04817, 1e-5);
//  EXPECT_NEAR(static_cast<double>(weights[3].At(0, 2)), -0.01474, 1e-5);
//}
