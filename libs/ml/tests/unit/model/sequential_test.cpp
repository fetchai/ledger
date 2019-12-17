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

#include "gtest/gtest.h"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/model/sequential.hpp"
#include "ml/ops/activation.hpp"
#include "ml/saveparams/saveable_params.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

namespace fetch {
namespace ml {
namespace test {

using SizeVector = fetch::math::SizeVector;

template <typename T>
class SequentialModelTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SequentialModelTest, math::test::HighPrecisionTensorFloatingTypes);
namespace sequential_details {
template <typename TypeParam>
void PrepareTestDataAndLabels1D(TypeParam &train_data, TypeParam &train_label)
{
  train_data  = TypeParam::FromString("0, 1, 0; 1, 0, 0; 0, 0, 1");
  train_label = TypeParam::FromString("0, 1, 2");
}

template <typename TypeParam, typename DataType, typename ModelType>
ModelType SetupModel(fetch::ml::OptimiserType                 optimiser_type,
                     fetch::ml::model::ModelConfig<DataType> &model_config, TypeParam &data,
                     TypeParam &gt)
{
  // setup dataloader
  using DataLoaderType = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>;
  auto data_loader_ptr = std::make_unique<DataLoaderType>();
  data_loader_ptr->AddData({data}, gt);

  // run model in training modeConfig
  auto model = ModelType(model_config);
  model.template Add<fetch::ml::layers::FullyConnected<TypeParam>>(
      3, 7, fetch::ml::details::ActivationType::RELU);
  model.template Add<fetch::ml::layers::FullyConnected<TypeParam>>(
      7, 5, fetch::ml::details::ActivationType::RELU);
  model.template Add<fetch::ml::layers::FullyConnected<TypeParam>>(5, 1);
  model.SetDataloader(std::move(data_loader_ptr));
  model.Compile(optimiser_type, fetch::ml::ops::LossType::MEAN_SQUARE_ERROR);

  return model;
}

template <typename TypeParam>
bool RunTest(fetch::ml::OptimiserType optimiser_type, typename TypeParam::Type tolerance,
             typename TypeParam::Type lr             = static_cast<typename TypeParam::Type>(0.5),
             fetch::math::SizeType    training_steps = 100)
{
  using DataType  = typename TypeParam::Type;
  using ModelType = fetch::ml::model::Sequential<TypeParam>;

  fetch::ml::model::ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = lr;
  model_config.learning_rate_param.exponential_decay_rate = DataType{0.99f};

  // set up data
  TypeParam train_data, train_labels;
  PrepareTestDataAndLabels1D<TypeParam>(train_data, train_labels);

  // set up model
  ModelType model = SetupModel<TypeParam, DataType, ModelType>(optimiser_type, model_config,
                                                               train_data, train_labels);
  // test loss decreases
  DataType loss{0};
  DataType later_loss{0};

  model.Train(1, loss);
  model.Train(training_steps);
  model.Train(1, later_loss);

  EXPECT_LE(later_loss, loss);

  // test prediction performance
  TypeParam pred;
  model.Predict(train_data, pred);
  EXPECT_TRUE(pred.AllClose(train_labels, tolerance, tolerance));

  return true;
}
}  // namespace sequential_details

TYPED_TEST(SequentialModelTest, adagrad_sequential)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(sequential_details::RunTest<TypeParam>(fetch::ml::OptimiserType::ADAGRAD,
                                                     DataType{1e-4f}, DataType{0.05f}, 400));
}

TYPED_TEST(SequentialModelTest, adam_sequential)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(sequential_details::RunTest<TypeParam>(fetch::ml::OptimiserType::ADAM,
                                                     DataType{1e-3f}, DataType{0.01f}, 400));
}

TYPED_TEST(SequentialModelTest, momentum_sequential)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(sequential_details::RunTest<TypeParam>(fetch::ml::OptimiserType::MOMENTUM,
                                                     DataType{1e-4f}, DataType{0.5f}, 200));
}

TYPED_TEST(SequentialModelTest, rmsprop_sequential)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(sequential_details::RunTest<TypeParam>(fetch::ml::OptimiserType::RMSPROP,
                                                     DataType{1e-2f}, DataType{0.006f}, 200));
}

TYPED_TEST(SequentialModelTest, sgd_sequential)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(sequential_details::RunTest<TypeParam>(fetch::ml::OptimiserType::SGD, DataType{1e-4f},
                                                     DataType{0.7f}, 400));
}

TYPED_TEST(SequentialModelTest, sgd_sequential_serialisation)
{

  using DataType                          = typename TypeParam::Type;
  fetch::ml::OptimiserType optimiser_type = fetch::ml::OptimiserType::SGD;
  auto                     tolerance      = DataType{0};
  auto                     lr             = DataType{0.5f};

  using DataType  = typename TypeParam::Type;
  using ModelType = fetch::ml::model::Sequential<TypeParam>;

  fetch::math::SizeType n_training_steps = 10;

  fetch::ml::model::ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = lr;
  model_config.learning_rate_param.exponential_decay_rate = DataType{0.99f};

  // set up data
  TypeParam train_data, train_labels;
  sequential_details::PrepareTestDataAndLabels1D<TypeParam>(train_data, train_labels);

  // set up model
  ModelType model = sequential_details::SetupModel<TypeParam, DataType, ModelType>(
      optimiser_type, model_config, train_data, train_labels);

  // test prediction performance
  TypeParam pred1({3, 1});
  TypeParam pred2({3, 1});

  // serialise the model
  fetch::serializers::MsgPackSerializer b;
  b << model;

  // deserialise the model
  b.seek(0);
  auto model2 = std::make_shared<fetch::ml::model::Sequential<TypeParam>>();
  b >> *model2;

  model.Predict(train_data, pred1);
  model2->Predict(train_data, pred2);

  // Test if deserialised model returns same results
  EXPECT_TRUE(pred1.AllClose(pred2, tolerance, tolerance));

  // Do one iteration step
  model2->Train(n_training_steps);
  model2->Predict(train_data, pred1);

  // Test if only one model is being trained
  EXPECT_FALSE(pred1.AllClose(pred2, tolerance, tolerance));

  model.Train(n_training_steps);
  model.Predict(train_data, pred2);

  // Test if both models returns same results after training
  EXPECT_TRUE(pred1.AllClose(pred2, tolerance, tolerance));
}

TYPED_TEST(SequentialModelTest, sequential_predict_without_dataloader)
{
  using ModelType = fetch::ml::model::Sequential<TypeParam>;
  using DataType  = typename TypeParam::Type;

  fetch::ml::model::ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = DataType{0.03f};
  model_config.learning_rate_param.exponential_decay_rate = DataType{0.99f};

  // set up data
  TypeParam train_data, train_labels;
  sequential_details::PrepareTestDataAndLabels1D<TypeParam>(train_data, train_labels);

  // run model in training modeConfig
  auto model = ModelType(model_config);
  model.template Add<fetch::ml::layers::FullyConnected<TypeParam>>(
      3, 7, fetch::ml::details::ActivationType::RELU);
  model.template Add<fetch::ml::layers::FullyConnected<TypeParam>>(
      7, 5, fetch::ml::details::ActivationType::RELU);
  model.template Add<fetch::ml::layers::FullyConnected<TypeParam>>(5, 1);

  model.Compile(fetch::ml::OptimiserType::ADAM, fetch::ml::ops::LossType::MEAN_SQUARE_ERROR);
  // Predicting without setting a dataloader is fine
  EXPECT_NO_FATAL_FAILURE(model.Predict(train_data, train_labels));
}
}  // namespace test
}  // namespace ml
}  // namespace fetch
