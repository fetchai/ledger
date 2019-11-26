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

/////
/// MODEL SERIALISATION TESTS
/////
TEST_F(MLTests, serialisation_model)
{
  static char const *model_serialise_src = R"(

      function build_model() : Model
        var model = Model("sequential");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 1u64);
        return model;
      endfunction

      function main()

        // set up data and labels
        var data_shape = Array<UInt64>(2);
        data_shape[0] = 10u64;
        data_shape[1] = 1000u64;
        var label_shape = Array<UInt64>(2);
        label_shape[0] = 1u64;
        label_shape[1] = 1000u64;
        var data = Tensor(data_shape);
        var label = Tensor(label_shape);

        // set up a model
        var model1 = build_model();
        var model2 = build_model();
        var model3 = build_model();
        var model4 = build_model();

        // compile the models with different optimisers and loss functions
        model1.compile("mse", "sgd");
        model2.compile("cel", "sgd");
        model3.compile("mse", "adam");
        model4.compile("cel", "adam");

        // train the models
        model1.fit(data, label, 32u64);
        model2.fit(data, label, 32u64);
        model3.fit(data, label, 32u64);
        model4.fit(data, label, 32u64);

        // evaluate performance
        var loss1 = model1.evaluate();
        var loss2 = model2.evaluate();
        var loss3 = model3.evaluate();
        var loss4 = model4.evaluate();

        // make a prediction
        var prediction1 = model1.predict(data);
        var prediction2 = model2.predict(data);
        var prediction3 = model3.predict(data);
        var prediction4 = model4.predict(data);

        // serialise model
        var model_state1 = State<Model>("model1");
        var model_state2 = State<Model>("model2");
        var model_state3 = State<Model>("model3");
        var model_state4 = State<Model>("model4");
        model_state1.set(model1);
        model_state2.set(model2);
        model_state3.set(model3);
        model_state4.set(model4);

      endfunction
    )";

  std::string const model_name1{"model1"};
  std::string const model_name2{"model2"};
  std::string const model_name3{"model3"};
  std::string const model_name4{"model4"};

  ASSERT_TRUE(toolkit.Compile(model_serialise_src));
  EXPECT_CALL(toolkit.observer(), Write(model_name1, _, _));
  EXPECT_CALL(toolkit.observer(), Write(model_name2, _, _));
  EXPECT_CALL(toolkit.observer(), Write(model_name3, _, _));
  EXPECT_CALL(toolkit.observer(), Write(model_name4, _, _));
  ASSERT_TRUE(toolkit.Run());

  static char const *model_deserialise_src = R"(
      function main()
        var model_state1 = State<Model>("model1");
        var model_state2 = State<Model>("model2");
        var model_state3 = State<Model>("model3");
        var model_state4 = State<Model>("model4");
        var model1 = model_state1.get();
        var model2 = model_state2.get();
        var model3 = model_state3.get();
        var model4 = model_state4.get();
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(model_deserialise_src));
  EXPECT_CALL(toolkit.observer(), Exists(model_name1));
  EXPECT_CALL(toolkit.observer(), Exists(model_name2));
  EXPECT_CALL(toolkit.observer(), Exists(model_name3));
  EXPECT_CALL(toolkit.observer(), Exists(model_name4));
  EXPECT_CALL(toolkit.observer(), Read(model_name1, _, _)).Times(::testing::Between(1, 2));
  EXPECT_CALL(toolkit.observer(), Read(model_name2, _, _)).Times(::testing::Between(1, 2));
  EXPECT_CALL(toolkit.observer(), Read(model_name3, _, _)).Times(::testing::Between(1, 2));
  EXPECT_CALL(toolkit.observer(), Read(model_name4, _, _)).Times(::testing::Between(1, 2));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, model_string_serialisation_test)
{
  static char const *graph_serialise_src = R"(

      function build_model() : Model
        var model = Model("sequential");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 1u64);
        return model;
      endfunction

      function main()

        // set up data and labels
        var data_shape = Array<UInt64>(2);
        data_shape[0] = 10u64;
        data_shape[1] = 1000u64;
        var label_shape = Array<UInt64>(2);
        label_shape[0] = 1u64;
        label_shape[1] = 1000u64;
        var data = Tensor(data_shape);
        var label = Tensor(label_shape);

        // set up a model
        var model1 = build_model();
        var model2 = build_model();
        var model3 = build_model();
        var model4 = build_model();
        // compile the models with different optimisers and loss functions
        model1.compile("mse", "sgd");
        model2.compile("cel", "sgd");
        model3.compile("mse", "adam");
        model4.compile("cel", "adam");

        // train the models
        model1.fit(data, label, 32u64);
        model2.fit(data, label, 32u64);
        model3.fit(data, label, 32u64);
        model4.fit(data, label, 32u64);

        // evaluate performance
        var loss1 = model1.evaluate();
        var loss2 = model2.evaluate();
        var loss3 = model3.evaluate();
        var loss4 = model4.evaluate();

        // make a prediction
        var prediction1 = model1.predict(data);
        var prediction2 = model2.predict(data);
        var prediction3 = model3.predict(data);
        var prediction4 = model4.predict(data);

       // serialise to string
        var model_string_1 = model1.serializeToString();
        var model_string_2 = model2.serializeToString();
        var model_string_3 = model3.serializeToString();
        var model_string_4 = model4.serializeToString();

        var state1 = State<String>("model_state1");
        var state2 = State<String>("model_state2");
        var state3 = State<String>("model_state3");
        var state4 = State<String>("model_state4");

        state1.set(model_string_1);
        state2.set(model_string_2);
        state3.set(model_string_3);
        state4.set(model_string_4);

      endfunction
  )";

  std::string const state_name1{"model_state1"};
  std::string const state_name2{"model_state2"};
  std::string const state_name3{"model_state3"};
  std::string const state_name4{"model_state4"};
  ASSERT_TRUE(toolkit.Compile(graph_serialise_src));
  EXPECT_CALL(toolkit.observer(), Write(state_name1, _, _));
  EXPECT_CALL(toolkit.observer(), Write(state_name2, _, _));
  EXPECT_CALL(toolkit.observer(), Write(state_name3, _, _));
  EXPECT_CALL(toolkit.observer(), Write(state_name4, _, _));
  ASSERT_TRUE(toolkit.Run());

  static char const *graph_deserialise_src = R"(
    function main()
      var state1 = State<String>("model_state1");
      var state2 = State<String>("model_state2");
      var state3 = State<String>("model_state3");
      var state4 = State<String>("model_state4");

      var model_string1 = state1.get();
      var model_string2 = state2.get();
      var model_string3 = state3.get();
      var model_string4 = state4.get();

      var model1 = Model("none");
      var model2 = Model("none");
      var model3 = Model("none");
      var model4 = Model("none");
      model1 = model1.deserializeFromString(model_string1);
      model2 = model2.deserializeFromString(model_string2);
      model3 = model3.deserializeFromString(model_string3);
      model4 = model4.deserializeFromString(model_string4);

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(graph_deserialise_src));
  EXPECT_CALL(toolkit.observer(), Exists(state_name1));
  EXPECT_CALL(toolkit.observer(), Exists(state_name2));
  EXPECT_CALL(toolkit.observer(), Exists(state_name3));
  EXPECT_CALL(toolkit.observer(), Exists(state_name4));
  EXPECT_CALL(toolkit.observer(), Read(state_name1, _, _)).Times(::testing::Between(1, 2));
  EXPECT_CALL(toolkit.observer(), Read(state_name2, _, _)).Times(::testing::Between(1, 2));
  EXPECT_CALL(toolkit.observer(), Read(state_name3, _, _)).Times(::testing::Between(1, 2));
  EXPECT_CALL(toolkit.observer(), Read(state_name4, _, _)).Times(::testing::Between(1, 2));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, non_permitted_serialisation_model_sequential_test)
{
  static char const *model_sequential_serialise_src = R"(

      function main()

        // set up data and labels
        var data_shape = Array<UInt64>(2);
        data_shape[0] = 10u64;
        data_shape[1] = 1000u64;
        var label_shape = Array<UInt64>(2);
        label_shape[0] = 1u64;
        label_shape[1] = 1000u64;
        var data = Tensor(data_shape);
        var label = Tensor(label_shape);

        // set up a model
        var model = Model("sequential");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 1u64);

        // serialise model
        var model_state = State<Model>("model");
        model_state.set(model);

      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(model_sequential_serialise_src));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MLTests, non_permitted_serialisation_model_regressor_test)
{
  static char const *model_regressor_serialise_src = R"(

      function main()

        // set up data and labels
        var data_shape = Array<UInt64>(2);
        data_shape[0] = 10u64;
        data_shape[1] = 1000u64;
        var label_shape = Array<UInt64>(2);
        label_shape[0] = 1u64;
        label_shape[1] = 1000u64;
        var data = Tensor(data_shape);
        var label = Tensor(label_shape);

        // set up a model
        var model = Model("regressor");

        // serialise model
        var model_state = State<Model>("model");
        model_state.set(model);

      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(model_regressor_serialise_src));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(MLTests, non_permitted_serialisation_model_classifier_test)
{
  static char const *model_classifier_serialise_src = R"(

      function main()

        // set up data and labels
        var data_shape = Array<UInt64>(2);
        data_shape[0] = 10u64;
        data_shape[1] = 1000u64;
        var label_shape = Array<UInt64>(2);
        label_shape[0] = 1u64;
        label_shape[1] = 1000u64;
        var data = Tensor(data_shape);
        var label = Tensor(label_shape);

        // set up a model
        var model = Model("classifier");

        // serialise model
        var model_state = State<Model>("model");
        model_state.set(model);

      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(model_classifier_serialise_src));
  EXPECT_FALSE(toolkit.Run());
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

TEST_F(MLTests, dense_sequential_model_test)
{
  static char const *sequential_model_src = R"(
    function main()

      // set up data and labels
      var data_shape = Array<UInt64>(2);
      data_shape[0] = 10u64;
      data_shape[1] = 1000u64;
      var label_shape = Array<UInt64>(2);
      label_shape[0] = 1u64;
      label_shape[1] = 1000u64;
      var data = Tensor(data_shape);
      var label = Tensor(label_shape);

      // set up a model
      var model = Model("sequential");
      model.add("dense", 10u64, 10u64, "relu");
      model.add("dense", 10u64, 10u64, "relu");
      model.add("dense", 10u64, 1u64);
      model.compile("mse", "adam");

      // train the model
      model.fit(data, label, 32u64);

      // make a prediction
      var loss = model.evaluate();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(sequential_model_src));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, conv1d_sequential_model_test)
{
  static char const *sequential_model_src = R"(
    function main() : Tensor

      // conv1d parameters
      var input_channels  = 3u64;
      var output_channels = 5u64;
      var input_height    = 3u64;
      var kernel_size     = 3u64;
      var output_height   = 1u64;
      var stride_size     = 1u64;

      // set up input data tensor
      var data_shape = Array<UInt64>(3);
      data_shape[0] = input_channels;
      data_shape[1] = input_height;
      data_shape[2] = 1u64;
      var data = Tensor(data_shape);
      for (in_channel in 0u64:input_channels)
        for (in_height in 0u64:input_height)
          data.setAt(in_channel, in_height, 0u64, toFixed64(in_height + 1u64));
        endfor
      endfor

      // set up a gt label tensor
      var label_shape = Array<UInt64>(3);
      label_shape[0] = output_channels;
      label_shape[1] = output_height;
      label_shape[2] = 1u64;
      var label = Tensor(label_shape);

      // set up a model
      var model = Model("sequential");
      model.add("conv1d", output_channels, input_channels, kernel_size, stride_size);
      model.compile("mse", "adam");

      // make an initial prediction
      var prediction = model.predict(data);

      // train the model
      model.fit(data, label, 1u64);

      // evaluate performance
      var loss = model.evaluate();

      return prediction;
    endfunction
  )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(sequential_model_src));
  ASSERT_TRUE(toolkit.Run(&res));
  auto const prediction = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();

  fetch::math::Tensor<fetch::vm_modules::math::DataType> gt({5, 1});
  gt(0, 0) = static_cast<DataType>(+7.29641703);
  gt(1, 0) = static_cast<DataType>(+5.42749771);
  gt(2, 0) = static_cast<DataType>(+1.89785659);
  gt(3, 0) = static_cast<DataType>(-0.52079467);
  gt(4, 0) = static_cast<DataType>(+0.57897364);

  ASSERT_TRUE((prediction->GetTensor())
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
}

TEST_F(MLTests, conv2d_sequential_model_test)
{
  static char const *sequential_model_src = R"(
    function main() : Tensor

      // conv1d parameters
      var input_channels  = 3u64;
      var output_channels = 5u64;
      var input_height    = 3u64;
      var input_width     = 3u64;
      var kernel_size     = 3u64;
      var output_height   = 1u64;
      var output_width    = 1u64;
      var stride_size     = 1u64;

      // set up input data tensor
      var data_shape = Array<UInt64>(4);
      data_shape[0] = input_channels;
      data_shape[1] = input_height;
      data_shape[2] = input_width;
      data_shape[3] = 1u64;
      var data = Tensor(data_shape);
      for (in_channel in 0u64:input_channels)
        for (in_height in 0u64:input_height)
          for (in_width in 0u64:input_width)
            data.setAt(in_channel, in_height, in_width, 0u64, toFixed64(in_height * in_width + 1u64));
          endfor
        endfor
      endfor

      // set up a gt label tensor
      var label_shape = Array<UInt64>(4);
      label_shape[0] = output_channels;
      label_shape[1] = output_height;
      label_shape[2] = output_width;
      label_shape[3] = 1u64;
      var label = Tensor(label_shape);

      // set up a model
      var model = Model("sequential");
      model.add("conv2d", output_channels, input_channels, kernel_size, stride_size);
      model.compile("mse", "adam");

      // make an initial prediction
      var prediction = model.predict(data);

      // train the model
      model.fit(data, label, 1u64);

      // evaluate performance
      var loss = model.evaluate();

      return prediction;
    endfunction
  )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(sequential_model_src));
  ASSERT_TRUE(toolkit.Run(&res));
  auto const prediction = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();

  fetch::math::Tensor<fetch::vm_modules::math::DataType> gt({5, 1, 1});
  gt.Set(0, 0, 0, static_cast<DataType>(+2.96216551));
  gt.Set(1, 0, 0, static_cast<DataType>(+10.21055092));
  gt.Set(2, 0, 0, static_cast<DataType>(-2.11563497));
  gt.Set(3, 0, 0, static_cast<DataType>(+1.88992180));
  gt.Set(4, 0, 0, static_cast<DataType>(+14.14585049));

  ASSERT_TRUE((prediction->GetTensor())
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
}

TEST_F(MLTests, classifier_model_test)
{
  static char const *classifier_model_src = R"(
    function main()

      // set up data and labels
      var data_shape = Array<UInt64>(2);
      data_shape[0] = 10u64;
      data_shape[1] = 1000u64;
      var label_shape = Array<UInt64>(2);
      label_shape[0] = 10u64;
      label_shape[1] = 1000u64;
      var data = Tensor(data_shape);
      var label = Tensor(label_shape);

      // set up a model
      var hidden_layers = Array<UInt64>(3);
      hidden_layers[0] = 10u64;
      hidden_layers[1] = 10u64;
      hidden_layers[2] = 10u64;
      var model = Model("classifier");
      model.compile("adam", hidden_layers);

      // train the model
      model.fit(data, label, 32u64);

      // make a prediction
      var loss = model.evaluate();

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(classifier_model_src));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(MLTests, regressor_model_test)
{
  static char const *regressor_model_src = R"(
    function main()

      // set up data and labels
      var data_shape = Array<UInt64>(2);
      data_shape[0] = 10u64;
      data_shape[1] = 1000u64;
      var label_shape = Array<UInt64>(2);
      label_shape[0] = 1u64;
      label_shape[1] = 1000u64;
      var data = Tensor(data_shape);
      var label = Tensor(label_shape);

      // set up a model
      var hidden_layers = Array<UInt64>(3);
      hidden_layers[0] = 10u64;
      hidden_layers[1] = 10u64;
      hidden_layers[2] = 1u64;
      var model = Model("regressor");
      model.compile("adam", hidden_layers);

      // train the model
      model.fit(data, label, 32u64);

      // make a prediction
      var loss = model.evaluate();

    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(regressor_model_src));
  ASSERT_TRUE(toolkit.Run());
}
}  // namespace
