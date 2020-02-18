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

#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/test_utilities/vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <regex>

using namespace fetch::vm;

namespace {

using DataType = fetch::vm_modules::math::DataType;

std::string const BOSTON_HOUSING_SOURCE = R"(
    function main()

      // read in training and test data
      var data = Tensor('0.00632,18.0;2.31,0.0;0.538,6.575;65.2,4.09;1.0,296.0;15.3,396.9;4.98,0.00632;18.0,2.31;0.0,0.538;6.575,65.2;4.09,1.0;296.0,15.3;396.9,4.98;');
      var label = Tensor('24.0,24.0;');

      // set up a model architecture
      var model = Model("sequential");
      model.add("dense", 13u64, 10u64, "relu");
      model.add("dense", 10u64, 10u64, "relu");
      model.add("dense", 10u64, 1u64);
      model.compile("mse", "adam");

      // train the model
      var batch_size = 2u64;
      model.fit(data, label, batch_size);

      // evaluate performance
      var loss = model.evaluate();

      // make predictions on data
      var predictions = model.predict(data);

    endfunction
  )";

constexpr bool IGNORE_CHARGE_ESTIMATION = true;

using VMPtr = std::shared_ptr<fetch::vm::VM>;

class VMBostonTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(VMBostonTests, model_add_dense_noact)
{
  std::string const src = BOSTON_HOUSING_SOURCE;
  ASSERT_TRUE(toolkit.Compile(src));
  if (IGNORE_CHARGE_ESTIMATION)
  {
    ASSERT_TRUE(toolkit.Run(nullptr, ChargeAmount{0}));
  }
  else
  {
    ASSERT_TRUE(toolkit.Run());
  }
}

}  // namespace
