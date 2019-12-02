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

std::string const ADD_INVALID_LAYER_TEST_SOURCE = R"(
    function main()
      var model = Model("sequential");
      <<TOKEN>>
    endfunction
  )";

class VMModelTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};

  void TestInvalidLayerAdding(std::string const &test_case_source)
  {
    std::string const src = std::regex_replace(ADD_INVALID_LAYER_TEST_SOURCE,
                                               std::regex("<<TOKEN>>"), test_case_source);
    ASSERT_TRUE(toolkit.Compile(src));
    // Invalid layer adding parameters (activation, layer type, parameter values) must not cause
    // unhandled exception/runtime crash, but should raise VM RuntimeError and cause a safe stop.
    ASSERT_FALSE(toolkit.Run());
  }

  void TestAddingUncompilableLayer(std::string const &test_case_source)
  {
    std::string const src = std::regex_replace(ADD_INVALID_LAYER_TEST_SOURCE,
                                               std::regex("<<TOKEN>>"), test_case_source);
    // Wrong number of arguments in layer adding parameters or calling uncompatible ".compile()"
    // method for a model must end in model compilation error and safe stop.
    ASSERT_FALSE(toolkit.Compile(src));
  }
};  // namespace

TEST_F(VMModelTests, serialisation_model)
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

TEST_F(VMModelTests, model_string_serialisation_test)
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

TEST_F(VMModelTests, non_permitted_serialisation_model_sequential_test)
{
  static char const *model_sequential_serialise_src = R"(

      function main()

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

TEST_F(VMModelTests, non_permitted_serialisation_model_regressor_test)
{
  static char const *model_regressor_serialise_src = R"(

      function main()

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

TEST_F(VMModelTests, non_permitted_serialisation_model_classifier_test)
{
  static char const *model_classifier_serialise_src = R"(

      function main()

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

TEST_F(VMModelTests, model_init_with_wrong_name)
{
  static char const *SRC_CORRECT_NAMES = R"(
        function main()
          var model1 = Model("sequential");
          var model2 = Model("regressor");
          var model3 = Model("classifier");
          var model4 = Model("none");
        endfunction
      )";
  ASSERT_TRUE(toolkit.Compile(SRC_CORRECT_NAMES));
  EXPECT_TRUE(toolkit.Run());

  static char const *SRC_WRONG_NAME = R"(
      function main()
        var model = Model("wrong_name");
      endfunction
    )";
  ASSERT_TRUE(toolkit.Compile(SRC_WRONG_NAME));
  EXPECT_THROW(toolkit.Run(), std::runtime_error);
}

TEST_F(VMModelTests, model_add_invalid_layer_type)
{
  TestInvalidLayerAdding(R"(model.add("INVALID_LAYER_TYPE", 1u64, 1u64, 1u64, 1u64);)");
}

TEST_F(VMModelTests, model_add_dense_invalid_params_noact)
{
  TestInvalidLayerAdding(R"(model.add("dense", 1u64, 1u64, 1u64, 1u64);)");
}

TEST_F(VMModelTests, model_add_dense_invalid_params_relu)
{
  TestInvalidLayerAdding(R"(model.add("dense", 1u64, 1u64, 1u64, 1u64, "relu");)");
}

TEST_F(VMModelTests, model_add_conv_invalid_params_noact)
{
  TestInvalidLayerAdding(R"(model.add("conv1d", 10u64, 10u64);)");
}

TEST_F(VMModelTests, model_add_conv_invalid_params_relu)
{
  TestInvalidLayerAdding(R"(model.add("conv1d", 10u64, 10u64, "relu");)");
}

TEST_F(VMModelTests, model_add_layers_invalid_activation_dense)
{
  TestInvalidLayerAdding(R"(model.add("dense", 10u64, 10u64, "INVALID_ACTIVATION_DENSE");)");
}

TEST_F(VMModelTests, model_add_layers_invalid_activation_conv)
{
  TestInvalidLayerAdding(
      R"(model.add("conv1d", 1u64, 1u64, 1u64, 1u64, "INVALID_ACTIVATION_CONV");)");
}

TEST_F(VMModelTests, model_uncompilable_add_layer__dense_uncompatible_params)
{
  TestAddingUncompilableLayer(R"(model.add("dense", 10u64, 10u64, 10u64, "relu");)");
}

TEST_F(VMModelTests, model_uncompilable_add_layer__conv_uncompatible_params)
{
  TestAddingUncompilableLayer(R"(model.add("conv1d", 10u64, 10u64, 10u64, "relu");)");
}

TEST_F(VMModelTests, model_uncompilable_add_layer__dense_invalid_params)
{
  TestAddingUncompilableLayer(R"(model.add("dense", 10fp32, 10u64, "relu");)");
}

TEST_F(VMModelTests, model_uncompilable_add_layer__conv_invalid_params)
{
  TestAddingUncompilableLayer(R"(model.add("conv1d", 0u64, 10fp32, 10u64, 10u64, "relu");)");
}

TEST_F(VMModelTests, model_add_layer_to_non_sequential)
{
  static char const *SRC = R"(
        function main()
          var model = Model("regressor");
          model.add("conv1d", 1u64, 1u64, 1u64, 1u64);
        endfunction
      )";
  ASSERT_TRUE(toolkit.Compile(SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_empty_sequential_compilation)
{
  static char const *EMPTY_SEQUENTIAL_SRC = R"(
      function main()
         var model = Model("sequential");
         model.compile("mse", "sgd");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(EMPTY_SEQUENTIAL_SRC));
  std::cout << "Testing compilation of an empty Sequential model" << std::endl;
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_compilation_invalid_params)
{
  static char const *SEQUENTIAL_SRC = R"(
      function main()
         var model = Model("sequential");
         model.add("dense", 10u64, 1u64);
         <<TOKEN>>
      endfunction
    )";

  static char const *INVALID_LOSS      = R"(model.compile("INVALID_LOSS", "adam");)";
  static char const *INVALID_OPTIMIZER = R"(model.compile("mse", "INVALID_OPTIMIZER");)";
  static char const *INVALID_BOTH      = R"(model.compile("INVALID_LOSS", "INVALID_OPTIMIZER");)";

  for (auto const &test_case : {INVALID_LOSS, INVALID_OPTIMIZER, INVALID_BOTH})
  {
    std::cout << "Testing invalid model compilation params: " << test_case << std::endl;
    std::string const src = std::regex_replace(SEQUENTIAL_SRC, std::regex("<<TOKEN>>"), test_case);
    ASSERT_TRUE(toolkit.Compile(src));
    EXPECT_FALSE(toolkit.Run());
  }
}

TEST_F(VMModelTests, model_compilation_simple_with_wrong_optimizer)
{
  static char const *SIMPLE_NONADAM_SRC = R"(
      function main()
         var hidden_layers = Array<UInt64>(2);
         var model = Model("classifier");
         model.compile("sgd", hidden_layers);
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SIMPLE_NONADAM_SRC));
  std::cout << "Testing non-Adam optimizer for a Simple model" << std::endl;
  EXPECT_FALSE(toolkit.Run());
}
TEST_F(VMModelTests, model_compilation_simple_with_too_few_layer_shapes)
{
  static char const *SIMPLE_1_HIDDEN_SRC = R"(
      function main()
         var hidden_layers = Array<UInt64>(1);
         var model = Model("classifier");
         model.compile("adam", hidden_layers);
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SIMPLE_1_HIDDEN_SRC));
  std::cout << "Testing insufficient hidden layers quantity for a Simple model" << std::endl;
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_compilation_sequential_from_layer_shapes)
{
  static char const *HIDDEN_TO_SEQUENTIAL_SRC = R"(
      function main()
         var hidden_layers = Array<UInt64>(10);
         var model = Model("sequential");
         model.compile("adam", hidden_layers);
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(HIDDEN_TO_SEQUENTIAL_SRC));
  std::cout << "Testing passing of hidden layers to Sequential model" << std::endl;
  EXPECT_FALSE(toolkit.Run());
}  // namespace

TEST_F(VMModelTests, dense_sequential_model_test)
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

TEST_F(VMModelTests, conv1d_sequential_model_test)
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

TEST_F(VMModelTests, conv2d_sequential_model_test)
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

TEST_F(VMModelTests, classifier_model_test)
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

TEST_F(VMModelTests, regressor_model_test)
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
