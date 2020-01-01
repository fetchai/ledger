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
#include "vm/pair.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_test_toolkit.hpp"

#include <regex>
#include <sstream>
#include <string>

using namespace fetch::vm;

namespace {

using DataType = fetch::vm_modules::math::DataType;

/// A minimal compileable etch code to test construction of an Optimizer.
/// Note: the constructed optimiser can not be used.
const char *OPTIMIZER_MINIMAL_CONSTRUCTION = R"(
     function main()
         var graph = Graph();
         var dataloader = DataLoader("tensor");
         var optimiser = Optimiser("%NAME%", graph, dataloader, {"",""}, "", "");
     endfunction
  )";

class MLTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};

  void TestOptimizerConstruction(std::string const &name)
  {
    std::string const src =
        std::regex_replace(OPTIMIZER_MINIMAL_CONSTRUCTION, std::regex("%NAME%"), name);
    ASSERT_TRUE(toolkit.Compile(src));
    ASSERT_TRUE(toolkit.Run());
  }
};

TEST_F(MLTests, DISABLED_dataloader_commodity_construction)
{
  static char const *SOURCE = R"(
    function main()
      var dataloader = DataLoader("commodity");
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, DISABLED_dataloader_tensor_construction)
{
  static char const *SOURCE = R"(
    function main()
      var dataloader = DataLoader("tensor");
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, DISABLED_dataloader_invalid_mode_construction)
{
  static char const *SOURCE = R"(
    function main()
      var dataloader = DataLoader("INVALID_MODE");
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(MLTests, DISABLED_dataloader_commodity_invalid_serialisation)
{
  static char const *SOURCE = R"(
    function main()
      var dataloader = DataLoader("commodity");
      var state = State<DataLoader>("dataloader");
      state.set(dataloader);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MLTests, DISABLED_dataloader_tensor_serialisation_test)
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

TEST_F(MLTests, DISABLED_trivial_persistent_tensor_dataloader_serialisation_test)
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

TEST_F(MLTests, DISABLED_dataloader_commodity_mode_invalid_add_data_by_tensor)
{
  static char const *SOURCE = R"(
    function main()
        var tensor_shape = Array<UInt64>(1);
        tensor_shape[0] = 1u64;
        var data_tensor = Tensor(tensor_shape);
        var label_tensor = Tensor(tensor_shape);
        var dataloader = DataLoader("commodity");
        dataloader.addData({data_tensor}, label_tensor);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(MLTests, DISABLED_dataloader_tensor_mode_invalid_add_data_by_files)
{
  static char const *SOURCE = R"(
    function main()
        var dataloader = DataLoader("tensor");
        dataloader.addData("x_filename", "y_filename");
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(MLTests, DISABLED_dataloader_serialisation_test)
{
  static char const *dataloader_serialise_src = R"(
    function main() : Pair<Tensor,Array<Tensor>>

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
      function main() : Pair<Tensor,Array<Tensor>>
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

  auto const initial_training_pair = first_res.Get<fetch::vm::Ptr<fetch::vm::Pair<
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>,
      fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>>>>>();
  auto const training_pair         = res.Get<fetch::vm::Ptr<fetch::vm::Pair<
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>,
      fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>>>>>();

  AnyInteger index(0, TypeIds::UInt16);

  auto array1 =
      initial_training_pair->GetSecond()
          .Get<
              fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>>>()
          ->GetIndexedValue(index);
  auto array2 =
      training_pair->GetSecond()
          .Get<
              fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>>>()
          ->GetIndexedValue(index);

  auto data1 = array1.Get<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>()->GetTensor();
  auto data2 = array2.Get<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>()->GetTensor();

  auto label1 = initial_training_pair->GetFirst()
                    .Get<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>()
                    ->GetTensor();
  auto label2 = training_pair->GetFirst()
                    .Get<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>()
                    ->GetTensor();

  EXPECT_TRUE(data1.AllClose(data2, DataType{0}, DataType{0}));
  EXPECT_TRUE(label1.AllClose(label2, DataType{0}, DataType{0}));
}

TEST_F(MLTests, DISABLED_graph_serialisation_test)
{
  static char const *graph_serialise_src = R"(
    persistent graph_state : Graph;
    function main() : Tensor
      use graph_state;

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

      graph_state.set(graph);

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
      use graph_state;

      var tensor_shape = Array<UInt64>(2);
      tensor_shape[0] = 2u64;
      tensor_shape[1] = 10u64;
      var data_tensor = Tensor(tensor_shape);
      var label_tensor = Tensor(tensor_shape);
      data_tensor.fill(7.0fp64);
      label_tensor.fill(7.0fp64);

      var graph = graph_state.get();

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

TEST_F(MLTests, DISABLED_graph_string_serialisation_test)
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

TEST_F(MLTests, DISABLED_optimiser_construction_adam)
{
  TestOptimizerConstruction("adam");
}

TEST_F(MLTests, DISABLED_optimiser_construction_adagrad)
{
  TestOptimizerConstruction("adagrad");
}

TEST_F(MLTests, DISABLED_optimiser_construction_rmsprop)
{
  TestOptimizerConstruction("rmsprop");
}

TEST_F(MLTests, DISABLED_optimiser_construction_sgd)
{
  TestOptimizerConstruction("sgd");
}

TEST_F(MLTests, DISABLED_optimiser_construction_invalid_type)
{
  std::string const src =
      std::regex_replace(OPTIMIZER_MINIMAL_CONSTRUCTION, std::regex("%NAME%"), "INVALID_NAME");
  ASSERT_TRUE(toolkit.Compile(src));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(MLTests, DISABLED_optimiser_adagrad_serialisation_failed)
{
  static char const *SOURCE = R"(
      function main()
        var graph = Graph();
        var dataloader = DataLoader("tensor");
        var optimiser = Optimiser("adagrad", graph, dataloader, {"",""}, "", "");
        var state = State<Optimiser>("optimiser");
        state.set(optimiser);
     endfunction
   )";
  ASSERT_TRUE(toolkit.Compile(SOURCE));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MLTests, DISABLED_optimiser_sgd_serialisation)
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

TEST_F(MLTests, DISABLED_serialisation_several_components_test)
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

TEST_F(MLTests, DISABLED_optimiser_set_graph_test)
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

TEST_F(MLTests, DISABLED_graph_step_test)
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

  EXPECT_GT(loss_reduction->GetTensor().At(0, 0), DataType{0});
}

}  // namespace
