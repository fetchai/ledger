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

#include "math/base_types.hpp"
#include "ml/model/sequential.hpp"
#include "ml/optimisation/types.hpp"
#include "ml/utilities/mnist_utilities.hpp"

#include "gtest/gtest.h"

namespace {

template <typename T>
class MNistTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(MNistTest, fetch::math::test::TensorFloatingTypes, );

TYPED_TEST(MNistTest, one_pass_test)
{
  using namespace fetch::ml::model;
  using namespace fetch::ml::optimisers;
  using DataType = typename TypeParam::Type;

  fetch::math::SizeType N_DATA = 32;

  /// setup config ///
  ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = fetch::math::Type<DataType>("0.0001");
  model_config.learning_rate_param.exponential_decay_rate = fetch::math::Type<DataType>("0.97");

  // generate some random data for training and testing
  auto data_label_pair = fetch::ml::utilities::generate_dummy_data<TypeParam>(N_DATA);

  // build and compile and mnist classifier
  auto model = fetch::ml::utilities::setup_mnist_model<TypeParam>(
      model_config, data_label_pair.first, data_label_pair.second);

  // training loop
  DataType loss;
  model.Train(2, loss);

  // Run model on a test set
  DataType test_loss;
  model.Test(test_loss);
}
}  // namespace
