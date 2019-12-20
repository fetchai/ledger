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

#include "ml/model/dnn_regressor.hpp"

#include "gtest/gtest.h"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/saveparams/saveable_params.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

namespace fetch {
namespace ml {
namespace test {
using SizeVector = fetch::math::SizeVector;

template <typename T>
class DNNRegressorModelTest : public ::testing::Test
{
};

TYPED_TEST_CASE(DNNRegressorModelTest, math::test::HighPrecisionTensorFloatingTypes);

namespace regressor_details {
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

  // run model in training mode
  auto model = ModelType(model_config, {3, 7, 5, 1});
  model.SetDataloader(std::move(data_loader_ptr));
  model.Compile(optimiser_type);

  return model;
}

template <typename TypeParam>
bool RunTest(fetch::ml::OptimiserType optimiser_type, typename TypeParam::Type tolerance,
             typename TypeParam::Type lr = fetch::math::Type<typename TypeParam::Type>("0.5"),
             fetch::math::SizeType    training_steps = 100)
{
  using DataType  = typename TypeParam::Type;
  using ModelType = fetch::ml::model::DNNRegressor<TypeParam>;

  fetch::ml::model::ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = lr;
  model_config.learning_rate_param.exponential_decay_rate = fetch::math::Type<DataType>("0.99");

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

}  // namespace regressor_details

TYPED_TEST(DNNRegressorModelTest, adagrad_dnnregressor)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(regressor_details::RunTest<TypeParam>(fetch::ml::OptimiserType::ADAGRAD,
                                                    fetch::math::Type<DataType>("0.0001"),
                                                    fetch::math::Type<DataType>("0.05"), 400));
}

TYPED_TEST(DNNRegressorModelTest, adam_dnnregressor)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(regressor_details::RunTest<TypeParam>(fetch::ml::OptimiserType::ADAM,
                                                    fetch::math::Type<DataType>("0.001"),
                                                    fetch::math::Type<DataType>("0.01"), 400));
}

TYPED_TEST(DNNRegressorModelTest, momentum_dnnregressor)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(regressor_details::RunTest<TypeParam>(fetch::ml::OptimiserType::MOMENTUM,
                                                    fetch::math::Type<DataType>("0.0001"),
                                                    fetch::math::Type<DataType>("0.5"), 200));
}

TYPED_TEST(DNNRegressorModelTest, rmsprop_dnnregressor)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(regressor_details::RunTest<TypeParam>(fetch::ml::OptimiserType::RMSPROP,
                                                    fetch::math::Type<DataType>("0.01"),
                                                    fetch::math::Type<DataType>("0.03"), 400));
}

TYPED_TEST(DNNRegressorModelTest, sgd_dnnregressor)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(regressor_details::RunTest<TypeParam>(fetch::ml::OptimiserType::SGD,
                                                    fetch::math::Type<DataType>("0.0001"),
                                                    fetch::math::Type<DataType>("0.7"), 400));
}

TYPED_TEST(DNNRegressorModelTest, sgd_dnnregressor_serialisation)
{
  using DataType  = typename TypeParam::Type;
  using ModelType = fetch::ml::model::DNNRegressor<TypeParam>;

  fetch::math::SizeType    n_training_steps = 10;
  auto                     tolerance        = static_cast<DataType>(0);
  auto                     learning_rate    = fetch::math::Type<DataType>("0.06");
  fetch::ml::OptimiserType optimiser_type   = fetch::ml::OptimiserType::SGD;

  fetch::ml::model::ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = learning_rate;
  model_config.learning_rate_param.exponential_decay_rate = fetch::math::Type<DataType>("0.99");

  // set up data
  TypeParam train_data, train_labels;
  regressor_details::PrepareTestDataAndLabels1D<TypeParam>(train_data, train_labels);

  // set up model
  ModelType model = regressor_details::SetupModel<TypeParam, DataType, ModelType>(
      optimiser_type, model_config, train_data, train_labels);

  // test prediction performance
  TypeParam pred1({3, 3});
  TypeParam pred2({3, 3});

  // serialise the model
  fetch::serializers::MsgPackSerializer b;
  b << model;

  // deserialise the model
  b.seek(0);
  auto model2 = std::make_shared<fetch::ml::model::DNNRegressor<TypeParam>>();
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

}  // namespace test
}  // namespace ml
}  // namespace fetch
