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
#include "vm_modules/scripts/ml/boston_house_prices.hpp"
#include "vm_modules/test_utilities/vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <regex>

using namespace fetch::vm;

namespace {

using DataType = fetch::vm_modules::math::DataType;

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
  std::string batch_size = "8u64";
  std::string const src = vm_modules::scripts::ml::BostonHousingScript(batch_size);

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
