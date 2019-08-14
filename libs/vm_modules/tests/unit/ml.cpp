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

#include "vm_modules/math/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/training_pair.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>

using namespace fetch::vm;
using ::testing::Between;
namespace {

// using ::testing::Between;

using DataType = fetch::vm_modules::math::DataType;

class MLTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(MLTests, dataloader_serialisation_test)
{
  static char const *dataloader_serialise_src = R"(
    function main() : TrainingPair

      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var x = Tensor(tensor_shape);
      x.fill(7.0fp64);

      var dataloader = DataLoader();
      dataloader.addData("tensor", data_tensor, label_tensor);

      var state = State<DataLoader>("dataloader");
      state.set(dataloader);

      return dataloader.GetNext();

    endfunction
  )";

  std::string const state_name{"graph"};
  Variant           first_res;
  ASSERT_TRUE(toolkit.Compile(dataloader_serialise_src));

  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));
  ASSERT_TRUE(toolkit.Run(&first_res));

  static char const *dataloader_deserialise_src = R"(
    function main() : TrainingPair
      var state = State<DataLoader>("dataloader");
      DataLoader dataloader = state.get();
      return  dataloader.GetNext();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(dataloader_deserialise_src));

  Variant res;
  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(Between(1, 2));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const initial_training_pair = first_res.Get<Ptr<fetch::vm_modules::ml::VMTrainingPair>>();
  auto const training_pair         = res.Get<Ptr<fetch::vm_modules::ml::VMTrainingPair>>();

  //  EXPECT_TRUE(initial_training_pair.first().AllClose(loss->GetTensor()));
}

TEST_F(MLTests, graph_serialisation_test)
{
  static char const *dataloader_serialise_src = R"(
    function main() : Tensor

      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var x = Tensor(tensor_shape);
      x.fill(7.0fp64);

      var dataloader = DataLoader();
      dataloader.addData("tensor", data_tensor, label_tensor);

      var graph = Graph();
      graph.addPlaceholder("Input");
      graph.addPlaceholder("Label");
      graph.addRelu("Output", "Input");
      graph.addMeanSquareErrorLoss("Error", "Output", "Label");

      var state = State<Graph>("graph");
      state.set(graph);

      return graph.Evaluate("Error");

    endfunction
  )";

  std::string const state_name{"graph"};
  Variant           first_res;
  ASSERT_TRUE(toolkit.Compile(dataloader_serialise_src));

  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));
  ASSERT_TRUE(toolkit.Run(&first_res));

  static char const *dataloader_deserialise_src = R"(
    function main() : Tensor
      var state = State<Graph>("graph");
      Graph graph = state.get();
      return  graph.Evaluate("Error");
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(dataloader_deserialise_src));

  Variant res;
  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(Between(1, 2));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const initial_loss = first_res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const loss         = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();

  EXPECT_TRUE(initial_loss->GetTensor().AllClose(loss->GetTensor()));
}

}  // namespace
