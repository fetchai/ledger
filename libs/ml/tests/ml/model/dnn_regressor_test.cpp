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
#include "ml/model/dnn_regressor.hpp"

#include "gtest/gtest.h"

using SizeVector = fetch::math::SizeVector;

template <typename T>
class ModelsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(ModelsTest, MyTypes);

template <typename TypeParam>
void PrepareTestDataAndLabels1D(TypeParam &train_data, TypeParam &train_label,
                                TypeParam &test_datum, TypeParam &test_label)
{
  train_data  = TypeParam::FromString("1.1, 2.1, 3.1; 4.1, 5.1, 6.1; 7.1, 8.1, 9.1");
  train_label = TypeParam::FromString("0.1; 1.1; 2.1");
  test_datum  = TypeParam::FromString("10.1; 11.1; 12.1");
  test_label  = TypeParam::FromString("3.1");
}

template <typename TypeParam, typename DataType, typename ModelType>
ModelType SetupModel(fetch::ml::optimisers::OptimiserType     optimiser_type,
                     fetch::ml::model::ModelConfig<DataType> &model_config, TypeParam &data,
                     TypeParam &gt)
{
  // setup dataloader
  using DataLoaderType = fetch::ml::dataloaders::TensorDataLoader<TypeParam, TypeParam>;
  SizeVector              label_shape = {gt.shape().at(0), 1};
  std::vector<SizeVector> data_shape  = {{data.shape().at(0), 1}};
  auto data_loader_ptr                = std::make_unique<DataLoaderType>(label_shape, data_shape);
  data_loader_ptr->AddData(data, gt);

  // run model in training mode
  auto model = ModelType(model_config, {3, 100, 100, 1});
  model.SetDataloader(std::move(data_loader_ptr));
  model.Compile(optimiser_type);

  return model;
}

template <typename TypeParam>
bool RunTest(fetch::ml::optimisers::OptimiserType optimiser_type,
             typename TypeParam::Type             tolerance,
             typename TypeParam::Type             lr = static_cast<typename TypeParam::Type>(0.5))
{
  using DataType  = typename TypeParam::Type;
  using ModelType = fetch::ml::model::DNNRegressor<TypeParam>;

  fetch::math::SizeType n_training_steps = 10;

  fetch::ml::model::ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = lr;
  model_config.learning_rate_param.exponential_decay_rate = static_cast<DataType>(0.99);

  // set up data
  TypeParam train_data, train_labels, test_datum, test_label;
  PrepareTestDataAndLabels1D<TypeParam>(train_data, train_labels, test_datum, test_label);

  // set up model
  ModelType model = SetupModel<TypeParam, DataType, ModelType>(optimiser_type, model_config,
                                                               train_data, train_labels);

  // test loss decreases
  fetch::math::SizeType count{0};
  while (count < n_training_steps)
  {
    DataType loss{0};
    DataType later_loss{0};
    model.Train(1, loss);
    model.Train(1, later_loss);
    EXPECT_LE(later_loss, loss);
    count++;
  }

  model.Train(100);

  // test prediction performance
  TypeParam pred({3, 1});

  model.Train(100);
  model.Predict(test_datum, pred);

  EXPECT_TRUE(pred.AllClose(test_label, tolerance, tolerance));

  return true;
}

TYPED_TEST(ModelsTest, adagrad_dnnregressor)
{

  // TODO (1556) - ADAGRAD not currently working
  //  using DataType  = typename TypeParam::Type;
  //  ASSERT_TRUE(RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::ADAGRAD,
  //  static_cast<DataType>(1e-1)));
}
TYPED_TEST(ModelsTest, adam_dnnregressor)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::ADAM,
                                 static_cast<DataType>(1e-5),
                                 static_cast<typename TypeParam::Type>(0.1)));
}
TYPED_TEST(ModelsTest, momentum_dnnregressor)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::MOMENTUM,
                                 static_cast<DataType>(1e-5)));
}
TYPED_TEST(ModelsTest, rmsprop_dnnregressor)
{
  // TODO(1557) - RMSPROP diverges for fixed point
  //  using DataType = typename TypeParam::Type;
  //  ASSERT_TRUE(RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::RMSPROP,
  //                                 static_cast<DataType>(1e-5)));
}
TYPED_TEST(ModelsTest, sgd_dnnregressor)
{
  using DataType = typename TypeParam::Type;
  ASSERT_TRUE(
      RunTest<TypeParam>(fetch::ml::optimisers::OptimiserType::SGD, static_cast<DataType>(1e-1)));
}
