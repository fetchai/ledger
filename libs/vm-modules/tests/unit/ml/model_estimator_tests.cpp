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

  VmPtr             vm_ptr_layer_type{new fetch::vm::String(&toolkit.vm(), layer_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(&toolkit.vm(), type_id, model_type);
  VmModelEstimator  model_estimator(model);

  for (SizeType inputs = 0; inputs < 1000; inputs += 10)
  {
    for (SizeType outputs = 0; outputs < 1000; outputs += 10)
    {
      DataType val = (model_estimator.ADD_DENSE_INPUT_COEF() * inputs);
      val += model_estimator.ADD_DENSE_OUTPUT_COEF() * outputs;
      val += model_estimator.ADD_DENSE_QUAD_COEF() * inputs * outputs;
      val += model_estimator.ADD_DENSE_CONST_COEF();

      EXPECT_TRUE(model_estimator.LayerAddDense(vm_ptr_layer_type, inputs, outputs) ==
                  static_cast<ChargeAmount>(val));
    }
  }
}

}  // namespace
