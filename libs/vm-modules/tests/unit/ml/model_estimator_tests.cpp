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

using DataType = fetch::vm_modules::math::DataType;

class VMModelEstimatorTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};  // namespace

TEST_F(VMModelEstimatorTests, add_dense_layer_test)
{
  std::string model_type = "sequential";
  fetch::vm::TypeId type_id = 0;
  fetch::vm_modules::ml::model::VMModel model(&toolkit.vm(), type_id, model_type);
  fetch::vm_modules::ml::model::ModelEstimator model_estimator(model);

  for (fetch::math::SizeType inputs = 0; inputs < 1000; inputs += 10)
  {
    for (fetch::math::SizeType outputs = 0; outputs < 1000; outputs += 10)
    {
      model.AddLayer("dense", inputs, outputs);

      EXPECT_TRUE(model_estimator.LayerAddDense() == ((inputs * outputs) + outputs));
    }
  }
}

}  // namespace
