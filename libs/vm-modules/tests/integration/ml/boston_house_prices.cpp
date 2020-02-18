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

#include "../unit/vm_test_toolkit.hpp"

#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/type.hpp"
//#include "vm_modules/ml/dataloaders/dataloader.hpp"

#include "gmock/gmock.h"

#include <regex>
//#include <sstream>

using namespace fetch::vm;

namespace {

using DataType = fetch::vm_modules::math::DataType;

std::string const BOSTON_HOUSING_SOURCE = R"(
    function main()

//      // read in training and test data
//      var data = readCSV("../data/boston/boston_10_data.csv");
//      var label = readCSV("../data/boston/boston_10_label.csv");
//
//      // set up a model architecture
//      var model = Model("sequential");
//      model.add("dense", 13u64, 10u64, "relu");
//      model.add("dense", 10u64, 10u64, "relu");
//      model.add("dense", 10u64, 1u64);
//      model.compile("mse", "adam");
//
//      // train the model
//      var batch_size = 10u64;
//      model.fit(data, label, batch_size);
//
//      // evaluate performance
//      var loss = model.evaluate();
//      print(loss[0u64]);
//
//      // make predictions on data
//      var predictions = model.predict(data);
//      print(predictions.at(0u64, 0u64));

    endfunction
  )";

constexpr bool IGNORE_CHARGE_ESTIMATION = true;

using VMPtr = std::shared_ptr<fetch::vm::VM>;

class VMBostonTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};

  VMPtr vm;

  void SetUp() override
  {
    // setup the VM
    using VMFactory = fetch::vm_modules::VMFactory;
    auto module     = VMFactory::GetModule(fetch::vm_modules::VMFactory::USE_ALL);
    vm              = std::make_shared<fetch::vm::VM>(module.get());
  }

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
