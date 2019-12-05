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
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/training_pair.hpp"
#include "vm_test_toolkit.hpp"

#include <regex>
#include <sstream>

using namespace fetch::vm;

namespace {

using DataType = fetch::vm_modules::math::DataType;

std::string const SCALER_SET_SCALE_BY_DATA_SRC = R"(
  function main()
    var height = 2u64;
    var width = 4u64;
    var data_shape = Array<UInt64>(2);
    data_shape[0] = height;
    data_shape[1] = width;

    var data_tensor = Tensor(data_shape);
    data_tensor.fillRandom();

    var scaler = Scaler();
    scaler.setScale(data_tensor, "%TOKEN%");
  endfunction
  )";

std::string const SCALER_SET_SCALE_BY_RANGE_SRC = R"(
    function main()
      var scaler = Scaler();
      scaler.setScale(%TOKEN%);
    endfunction
  )";

class VMScalerTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};

  std::string Substitute(std::string const &source, std::string const &what)
  {
    return std::regex_replace(source, std::regex("%TOKEN%"), what);
  }
};

TEST_F(VMScalerTests, scaler_construction)
{
  static char const *SOURCE = R"(
        function main()
            var scaler = Scaler();
        endfunction
    )";
  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(VMScalerTests, scaler_setscale_minmax)
{
  ASSERT_TRUE(toolkit.Compile(Substitute(SCALER_SET_SCALE_BY_DATA_SRC, "min_max")));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(VMScalerTests, scaler_setscale_invalid_mode)
{
  ASSERT_TRUE(toolkit.Compile(Substitute(SCALER_SET_SCALE_BY_DATA_SRC, "INVALID_MODE")));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(VMScalerTests, scaler_setscale_valid_range)
{
  ASSERT_TRUE(toolkit.Compile(Substitute(SCALER_SET_SCALE_BY_RANGE_SRC, "0fp64, 1fp64")));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(VMScalerTests, scaler_setscale_invalid_range)
{
  // Minimum value here is bigger then maximum: should cause runtime error.
  ASSERT_TRUE(toolkit.Compile(Substitute(SCALER_SET_SCALE_BY_RANGE_SRC, "1fp64, 0fp64")));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(VMScalerTests, scaler_normalize)
{}

}  // namespace
