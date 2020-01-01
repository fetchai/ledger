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
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>

using namespace fetch::vm;

namespace {

using DataType = fetch::vm_modules::math::DataType;

class OpsTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(OpsTests, DISABLED_tranpose_test)
{
  using TypeParam = fetch::math::Tensor<DataType>;

  TypeParam gt = TypeParam::FromString("1, 2; 3, 4; 5, 6");

  static char const *src = R"(
    function main() : Tensor

      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 3u64;

      var data_tensor = Tensor(tensor_shape);

      var string_vals = "1, 2, 3, 4, 5, 6";
      data_tensor.fromString(string_vals);

      var graph = Graph();
      graph.addPlaceholder("Input");
      graph.addTranspose("Transpose", "Input");

      graph.setInput("Input", data_tensor);

      var result = graph.evaluate("Transpose");

      return result;
    endfunction
  )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(src));
  ASSERT_TRUE(toolkit.Run(&res));

  TypeParam result = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>()->GetTensor();

  EXPECT_EQ(result.shape().size(), 2);
  EXPECT_EQ(result.shape().at(0), 3);
  EXPECT_EQ(result.shape().at(1), 2);
  ASSERT_TRUE(result.AllClose(gt));
}

TEST_F(OpsTests, DISABLED_exp_test)
{
  using TypeParam = fetch::math::Tensor<DataType>;

  TypeParam gt = TypeParam::FromString(
      "2.71828182845904, 0.135335283236613, 20.0855369231877, 0.018315638888734, 148.413159102577, "
      "0.002478752176666");

  static char const *src = R"(
    function main() : Tensor

      var tensor_shape = Array<UInt64>(1);
      tensor_shape[0] = 6u64;

      var data_tensor = Tensor(tensor_shape);

      var string_vals = "1, -2, 3, -4, 5, -6";
      data_tensor.fromString(string_vals);

      var graph = Graph();
      graph.addPlaceholder("Input");
      graph.addExp("Exp", "Input");

      graph.setInput("Input", data_tensor);

      var result = graph.evaluate("Exp");

      return result;
    endfunction
  )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(src));
  ASSERT_TRUE(toolkit.Run(&res));

  TypeParam result = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>()->GetTensor();

  EXPECT_EQ(result.shape().size(), 1);
  EXPECT_EQ(result.shape().at(0), 6);

  ASSERT_TRUE(result.AllClose(gt));
}

}  // namespace
