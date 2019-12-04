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

#include "gmock/gmock.h"
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/ml/model/model_estimator.hpp"
#include "vm_test_toolkit.hpp"

#include <regex>
#include <sstream>

using namespace fetch::vm;

namespace {

using SizeType         = fetch::math::SizeType;
using DataType         = fetch::vm_modules::math::DataType;
using VmPtr            = fetch::vm::Ptr<fetch::vm::String>;
using VmModel          = fetch::vm_modules::ml::model::VMModel;
using VmModelEstimator = fetch::vm_modules::ml::model::ModelEstimator;
using DataType         = fetch::vm_modules::ml::model::ModelEstimator::DataType;

class VMModelEstimatorTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

// sanity check that estimator behaves as intended
TEST_F(VMModelEstimatorTests, add_dense_layer_test)
{
  std::string model_type = "sequential";
  std::string layer_type = "dense";
  SizeType min_input_size = 0;
  SizeType max_input_size = 1000;
  SizeType input_step = 10;
  SizeType min_output_size = 0;
  SizeType max_output_size = 1000;
  SizeType output_step = 10;

  VmPtr             vm_ptr_layer_type{new fetch::vm::String(&toolkit.vm(), layer_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(&toolkit.vm(), type_id, model_type);
  VmModelEstimator  model_estimator(model);

  for (SizeType inputs = min_input_size; inputs < max_input_size; inputs += input_step)
  {
    for (SizeType outputs = min_output_size; outputs < max_output_size; outputs += output_step)
    {
      DataType val = (VmModelEstimator::ADD_DENSE_INPUT_COEF() * inputs);
      val += VmModelEstimator::ADD_DENSE_OUTPUT_COEF() * outputs;
      val += VmModelEstimator::ADD_DENSE_QUAD_COEF() * inputs * outputs;
      val += VmModelEstimator::ADD_DENSE_CONST_COEF();

      EXPECT_TRUE(model_estimator.LayerAddDense(vm_ptr_layer_type, inputs, outputs) ==
                  static_cast<ChargeAmount>(val));
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(VMModelEstimatorTests, add_dense_layer_activation_test)
{
  std::string model_type = "sequential";
  std::string layer_type = "dense";
  std::string activation_type = "relu";

  SizeType min_input_size = 0;
  SizeType max_input_size = 1000;
  SizeType input_step = 10;
  SizeType min_output_size = 0;
  SizeType max_output_size = 1000;
  SizeType output_step = 10;

  VmPtr             vm_ptr_layer_type{new fetch::vm::String(&toolkit.vm(), layer_type)};
  VmPtr             vm_ptr_activation_type{new fetch::vm::String(&toolkit.vm(), activation_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(&toolkit.vm(), type_id, model_type);
  VmModelEstimator  model_estimator(model);

  for (SizeType inputs = min_input_size; inputs < max_input_size; inputs += input_step)
  {
    for (SizeType outputs = min_output_size; outputs < max_output_size; outputs += output_step)
    {
      DataType val = (VmModelEstimator::ADD_DENSE_INPUT_COEF() * inputs);
      val += VmModelEstimator::ADD_DENSE_OUTPUT_COEF() * outputs;
      val += VmModelEstimator::ADD_DENSE_QUAD_COEF() * inputs * outputs;
      val += VmModelEstimator::ADD_DENSE_CONST_COEF();

      EXPECT_TRUE(model_estimator.LayerAddDenseActivation(vm_ptr_layer_type, inputs, outputs, vm_ptr_activation_type) ==
                  static_cast<ChargeAmount>(val));
    }
  }
}
}  // namespace
