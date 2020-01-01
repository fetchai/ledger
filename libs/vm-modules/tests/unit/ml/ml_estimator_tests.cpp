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

#include "gmock/gmock.h"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_test_toolkit.hpp"

namespace {

using namespace fetch::vm;

class VMMLEstimatorTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

////////////////////////////////////////////////////////////////////////////////
// ML VM objects/operations without estimators should have an infinite charge //
////////////////////////////////////////////////////////////////////////////////

TEST_F(VMMLEstimatorTests, vmgraph_constructor_have_infinite_charge)
{
  static constexpr char const *TEXT = R"(
    function main()
      var graph = Graph();
    endfunction
  )";

  EXPECT_TRUE(toolkit.Compile(TEXT));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMMLEstimatorTests, vmdataloader_constructor_have_infinite_charge)
{
  static constexpr char const *TEXT = R"(
    function main()
      var data_loader = DataLoader("tensor");
    endfunction
  )";

  EXPECT_TRUE(toolkit.Compile(TEXT)) << stdout.str();
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMMLEstimatorTests, vmscaler_constructor_have_infinite_charge)
{
  static constexpr char const *TEXT = R"(
    function main()
      var graph = Scaler();
    endfunction
  )";

  EXPECT_TRUE(toolkit.Compile(TEXT)) << stdout.str();
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMMLEstimatorTests, vmoptimiser_constructor_have_infinite_charge)
{
  static constexpr char const *TEXT = R"(
    function main()
      var graph = Optimiser("sgd", Graph(), DataLoader("tensor"), {"Input_1","Input_2"}, "Label", "Error");
    endfunction
  )";

  EXPECT_TRUE(toolkit.Compile(TEXT)) << stdout.str();
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMMLEstimatorTests, load_mnist_images_have_infinite_charge)
{
  static constexpr char const *TEXT = R"(
    function main()
      loadMNISTImages("");
    endfunction
  )";

  EXPECT_TRUE(toolkit.Compile(TEXT)) << stdout.str();
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMMLEstimatorTests, load_mnist_labels_have_infinite_charge)
{
  static constexpr char const *TEXT = R"(
    function main()
      loadMNISTLabels("");
    endfunction
  )";

  EXPECT_TRUE(toolkit.Compile(TEXT)) << stdout.str();
  EXPECT_FALSE(toolkit.Run());
}

}  // namespace
