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

TEST_F(MLTests, trivial_tensor_dataloader_serialisation_test)
{
  static char const *dataloader_serialise_src = R"(
    function main()
      var dataloader = DataLoader("tensor");
      var state = State<DataLoader>("dataloader");
      state.set(dataloader);
    endfunction
  )";

  std::string const state_name{"dataloader"};
  ASSERT_TRUE(toolkit.Compile(dataloader_serialise_src));
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));
  ASSERT_TRUE(toolkit.Run());

  static char const *dataloader_deserialise_src = R"(
      function main()
        var state = State<DataLoader>("dataloader");
        var dataloader = state.get();
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(dataloader_deserialise_src));
  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(::testing::Between(1, 2));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, trivial_persistent_tensor_dataloader_serialisation_test)
{
  static char const *dataloader_serialise_src = R"(
    persistent dataloader_state : DataLoader;
    function main()
      use dataloader_state;
      var dataloader = dataloader_state.get(DataLoader("tensor"));
      dataloader_state.set(dataloader);
    endfunction
  )";

  std::string const state_name{"dataloader_state"};
  ASSERT_TRUE(toolkit.Compile(dataloader_serialise_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *dataloader_deserialise_src = R"(
      persistent dataloader_state : DataLoader;
      function main()
        use dataloader_state;
        var dataloader = dataloader_state.get();
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(dataloader_deserialise_src));
  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, trivial_commodity_dataloader_test)
{
  static char const *dataloader_serialise_src = R"(
    function main()
      var dataloader = DataLoader("commodity");
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(dataloader_serialise_src));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, dataloader_serialisation_test)
{
  static char const *dataloader_serialise_src = R"(
    function main() : TrainingPair

      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var data_tensor_1 = Tensor(tensor_shape);
      var data_tensor_2 = Tensor(tensor_shape);
      var label_tensor = Tensor(tensor_shape);
      data_tensor_1.fill(7.0fp64);
      data_tensor_2.fill(7.0fp64);
      label_tensor.fill(7.0fp64);

      var dataloader = DataLoader("tensor");
      dataloader.addData({data_tensor_1,data_tensor_2}, label_tensor);

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

  AnyInteger index(0, TypeIds::UInt16);

  auto array1 = initial_training_pair->data()->GetIndexedValue(index);
  auto array2 = training_pair->data()->GetIndexedValue(index);

  auto data1 = array1.Get<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>()->GetTensor();
  auto data2 = array2.Get<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>()->GetTensor();

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
      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var data_tensor = Tensor(tensor_shape);
      var label_tensor = Tensor(tensor_shape);
      data_tensor.fill(7.0fp64);
      label_tensor.fill(7.0fp64);

      var state = State<Graph>("graph");
      var graph = state.get();

      graph.setInput("Input", data_tensor);
      graph.setInput("Label", label_tensor);
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

TEST_F(MLTests, graph_string_serialisation_test)
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

      var graph_string = graph.serializeToString();

      var state = State<String>("graph_state");
      state.set(graph_string);

      return graph.evaluate("Error");

    endfunction
  )";

  std::string const state_name{"graph_state"};
  Variant           first_res;
  ASSERT_TRUE(toolkit.Compile(graph_serialise_src));

  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));
  ASSERT_TRUE(toolkit.Run(&first_res));

  static char const *graph_deserialise_src = R"(
    function main() : Tensor
      var state = State<String>("graph_state");
      var graph_string = state.get();

      var graph = Graph();
      graph = graph.deserializeFromString(graph_string);

      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var data_tensor = Tensor(tensor_shape);
      var label_tensor = Tensor(tensor_shape);
      data_tensor.fill(7.0fp64);
      label_tensor.fill(7.0fp64);

      graph.setInput("Input", data_tensor);
      graph.setInput("Label", label_tensor);

      return graph.evaluate("Error");
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
      var data_tensor_1 = Tensor(tensor_shape);
      var data_tensor_2 = Tensor(tensor_shape);
      var label_tensor = Tensor(tensor_shape);
      data_tensor_1.fill(7.0fp64);
      data_tensor_2.fill(7.0fp64);
      label_tensor.fill(7.0fp64);

      var graph = Graph();
      graph.addPlaceholder("Input_1");
      graph.addPlaceholder("Input_2");
      graph.addPlaceholder("Label");
      graph.addFullyConnected("FC1", "Input_2", 2, 2);
      graph.addRelu("Output", "FC1");
      graph.addMeanSquareErrorLoss("Error", "Output", "Label");

      var dataloader = DataLoader("tensor");
      dataloader.addData({data_tensor_1,data_tensor_2}, label_tensor);

      var batch_size = 8u64;
      var optimiser = Optimiser("sgd", graph, dataloader, {"Input_1","Input_2"}, "Label", "Error");

      var state = State<Optimiser>("optimiser");
      state.set(optimiser);

      var loss = optimiser.run(batch_size);
      return loss;

    endfunction
  )";

  std::string const state_name{"optimiser"};
  Variant           first_res;
  ASSERT_TRUE(toolkit.Compile(optimiser_serialise_src));
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));
  ASSERT_TRUE(toolkit.Run(&first_res));
  auto const loss1 = first_res.Get<fetch::fixed_point::fp64_t>();

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

  EXPECT_TRUE(loss1 == loss2);
}

TEST_F(MLTests, serialisation_several_components_test)
{
  static char const *several_serialise_src = R"(
      function main()

      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var data_tensor_1 = Tensor(tensor_shape);
      var data_tensor_2 = Tensor(tensor_shape);
      var label_tensor = Tensor(tensor_shape);
      data_tensor_1.fill(7.0fp64);
      data_tensor_2.fill(7.0fp64);
      label_tensor.fill(7.0fp64);

        var graph = Graph();
        graph.addPlaceholder("Input_1");
        graph.addPlaceholder("Input_2");
        graph.addPlaceholder("Label");
        graph.addFullyConnected("FC1", "Input_2", 2, 2);
        graph.addRelu("Output", "FC1");
        graph.addMeanSquareErrorLoss("Error", "Output", "Label");
        var graph_state = State<Graph>("graph");
        graph_state.set(graph);

        var dataloader = DataLoader("tensor");

        dataloader.addData({data_tensor_1,data_tensor_2}, label_tensor);
        var dataloader_state = State<DataLoader>("dataloader");
        dataloader_state.set(dataloader);

        var batch_size = 8u64;
        var optimiser = Optimiser("sgd", graph, dataloader, {"Input_1","Input_2"}, "Label", "Error");
        var optimiser_state = State<Optimiser>("optimiser");
        optimiser_state.set(optimiser);

      endfunction
    )";

  std::string const graph_name{"graph"};
  std::string const dl_name{"dataloader"};
  std::string const opt_name{"optimiser"};

  ASSERT_TRUE(toolkit.Compile(several_serialise_src));
  EXPECT_CALL(toolkit.observer(), Write(graph_name, _, _));
  EXPECT_CALL(toolkit.observer(), Write(dl_name, _, _));
  EXPECT_CALL(toolkit.observer(), Write(opt_name, _, _));
  ASSERT_TRUE(toolkit.Run());

  static char const *several_deserialise_src = R"(
      function main()
        var graph_state = State<Graph>("graph");
        var dataloader_state = State<DataLoader>("dataloader");
        var optimiser_state = State<Optimiser>("optimiser");

        var graph = graph_state.get();
        var dataloader = dataloader_state.get();
        var optimiser = optimiser_state.get();
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(several_deserialise_src));
  EXPECT_CALL(toolkit.observer(), Exists(graph_name));
  EXPECT_CALL(toolkit.observer(), Exists(dl_name));
  EXPECT_CALL(toolkit.observer(), Exists(opt_name));
  EXPECT_CALL(toolkit.observer(), Read(graph_name, _, _)).Times(::testing::Between(1, 2));
  EXPECT_CALL(toolkit.observer(), Read(dl_name, _, _)).Times(::testing::Between(1, 2));
  EXPECT_CALL(toolkit.observer(), Read(opt_name, _, _)).Times(::testing::Between(1, 2));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, optimiser_set_graph_test)
{
  static char const *several_serialise_src = R"(
      function main()

        var tensor_shape = Array<UInt64>(2);
        tensor_shape[0] = 2u64;
        tensor_shape[1] = 10u64;
        var data_tensor_1 = Tensor(tensor_shape);
        var data_tensor_2 = Tensor(tensor_shape);
        var label_tensor = Tensor(tensor_shape);
        data_tensor_1.fill(7.0fp64);
        data_tensor_2.fill(7.0fp64);
        label_tensor.fill(7.0fp64);

        var graph = Graph();
        graph.addPlaceholder("Input_1");
        graph.addPlaceholder("Input_2");
        graph.addPlaceholder("Label");
        graph.addFullyConnected("FC1", "Input_2", 2, 2);
        graph.addRelu("Output", "FC1");
        graph.addMeanSquareErrorLoss("Error", "Output", "Label");

        var dataloader = DataLoader("tensor");
        dataloader.addData({data_tensor_1,data_tensor_2}, label_tensor);

        var batch_size = 8u64;
        var optimiser = Optimiser("sgd", graph, dataloader, {"Input_1","Input_2"}, "Label", "Error");

        optimiser.setGraph(graph);
        optimiser.setDataloader(dataloader);

      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(several_serialise_src));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, graph_step_test)
{
  static char const *src = R"(
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
      graph.addFullyConnected("FC1", "Input", 2, 2);
      graph.addMeanSquareErrorLoss("Error", "FC1", "Label");

      graph.setInput("Input", data_tensor);
      graph.setInput("Label", label_tensor);

      var loss = graph.evaluate("Error");
      graph.backPropagate("Error");
      graph.step(0.01fp64);

      var loss_after_training = graph.evaluate("Error");

      loss.setAt(0u64, 0u64, loss.at(0u64, 0u64) - loss_after_training.at(0u64, 0u64));

      return loss;
    endfunction
  )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(src));
  ASSERT_TRUE(toolkit.Run(&res));

  auto const loss_reduction = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();

  EXPECT_GT(loss_reduction->GetTensor().At(0, 0), 0);
}

}  // namespace
