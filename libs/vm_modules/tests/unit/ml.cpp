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
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/training_pair.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>

using namespace fetch::vm;

namespace {

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
      var data_tensor = Tensor(tensor_shape);
      var label_tensor = Tensor(tensor_shape);
      data_tensor.fill(7.0fp64);
      label_tensor.fill(7.0fp64);

      var dataloader = DataLoader();
      dataloader.addData("tensor", data_tensor, label_tensor);

      var state = State<DataLoader>("dataloader");
      state.set(dataloader);

      var tp = dataloader.getNext();
      return tp;

    endfunction
  )";

  std::string const state_name{"dataloader"};
  Variant           first_res;
  ASSERT_TRUE(toolkit.Compile(dataloader_serialise_src));

  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));
  ASSERT_TRUE(toolkit.Run(&first_res));

  static char const *dataloader_deserialise_src = R"(
      function main() : TrainingPair
        var state = State<DataLoader>("dataloader");
        var dataloader = state.get();
        var tp = dataloader.getNext();
        return tp;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(dataloader_deserialise_src));

  Variant res;
  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(::testing::Between(1, 2));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const initial_training_pair = first_res.Get<Ptr<fetch::vm_modules::ml::VMTrainingPair>>();
  auto const training_pair         = res.Get<Ptr<fetch::vm_modules::ml::VMTrainingPair>>();

  auto data1 = initial_training_pair->data()->GetTensor();
  auto data2 = training_pair->data()->GetTensor();

  auto label1 = initial_training_pair->label()->GetTensor();
  auto label2 = training_pair->label()->GetTensor();

  EXPECT_TRUE(data1.AllClose(data2, static_cast<DataType>(0), static_cast<DataType>(0)));
  EXPECT_TRUE(label1.AllClose(label2, static_cast<DataType>(0), static_cast<DataType>(0)));
}

TEST_F(MLTests, graph_serialisation_test)
{
  static char const *graph_serialise_src = R"(
    function main() : Tensor

      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var data_tensor = Tensor(tensor_shape);
      var label_tensor = Tensor(tensor_shape);
      data_tensor.fill(7.0fp64);
      label_tensor.fill(7.0fp64);

      var graph = Graph();
      graph.addPlaceholder("Input");
      graph.addPlaceholder("Label");
      graph.addRelu("Output", "Input");
      graph.addMeanSquareErrorLoss("Error", "Output", "Label");

      graph.setInput("Input", data_tensor);
      graph.setInput("Label", label_tensor);

      var state = State<Graph>("graph");
      state.set(graph);

      return graph.evaluate("Error");

    endfunction
  )";

  std::string const state_name{"graph"};
  Variant           first_res;
  ASSERT_TRUE(toolkit.Compile(graph_serialise_src));

  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));
  ASSERT_TRUE(toolkit.Run(&first_res));

  static char const *graph_deserialise_src = R"(
    function main() : Tensor
      var state = State<Graph>("graph");
      var graph = state.get();
      var loss = graph.evaluate("Error");
      return loss;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(graph_deserialise_src));

  Variant res;
  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(::testing::Between(1, 2));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const initial_loss = first_res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const loss         = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();

  EXPECT_TRUE(initial_loss->GetTensor().AllClose(loss->GetTensor()));
}

TEST_F(MLTests, sgd_optimiser_serialisation_test)
{
  static char const *optimiser_serialise_src = R"(
    function main() : Fixed64

      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var data_tensor = Tensor(tensor_shape);
      var label_tensor = Tensor(tensor_shape);
      data_tensor.fill(7.0fp64);
      label_tensor.fill(7.0fp64);

      var graph = Graph();
      graph.addPlaceholder("Input");
      graph.addPlaceholder("Label");
      graph.addFullyConnected("FC1", "Input", 2, 2);
      graph.addRelu("Output", "FC1");
      graph.addMeanSquareErrorLoss("Error", "Output", "Label");

      var dataloader = DataLoader();
      dataloader.addData("tensor", data_tensor, label_tensor);

      var batch_size = 8u64;
      var optimiser = Optimiser("sgd", graph, dataloader, "Input", "Label", "Error");

      var state = State<Optimiser>("optimiser");
      state.set(optimiser);

      ////////////
      // TODO (1533) - this is necessary due to a bug
      // now make a totally new optimiser, graph and dataloader with identical properties
      // this is necessary because the optimiser data is not written at state.set time
      // therefore the internal states of the optimser after calling run will be saved
      // to the state
      ////////////

      var graph2 = Graph();
      graph2.addPlaceholder("Input");
      graph2.addPlaceholder("Label");
      graph2.addFullyConnected("FC1", "Input", 2, 2);
      graph2.addRelu("Output", "FC1");
      graph2.addMeanSquareErrorLoss("Error", "Output", "Label");

      var dataloader2 = DataLoader();
      dataloader2.addData("tensor", data_tensor, label_tensor);

      var optimiser2 = Optimiser("sgd", graph2, dataloader2, "Input", "Label", "Error");
      var loss = optimiser2.run(batch_size);
      return loss;

    endfunction
  )";

  std::string const state_name{"optimiser"};
  Variant           first_res;
  ASSERT_TRUE(toolkit.Compile(optimiser_serialise_src));
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));
  ASSERT_TRUE(toolkit.Run(&first_res));
  auto const loss1 = first_res.Get<fetch::fixed_point::fp64_t>();

  std::cout << "loss1: " << loss1 << std::endl;

  static char const *optimiser_deserialise_src = R"(
      function main() : Fixed64
        var state = State<Optimiser>("optimiser");
        var optimiser = state.get();
        var batch_size = 8u64;
        var loss = optimiser.run(batch_size);
        return loss;
      endfunction
    )";

  Variant second_res;
  ASSERT_TRUE(toolkit.Compile(optimiser_deserialise_src));
  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(::testing::Between(1, 2));
  ASSERT_TRUE(toolkit.Run(&second_res));

  auto const loss2 = second_res.Get<fetch::fixed_point::fp64_t>();

  std::cout << "loss2: " << loss2 << std::endl;
  EXPECT_TRUE(loss1 == loss2);
}

}  // namespace
