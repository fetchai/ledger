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

std::string const ADD_VALID_LAYER_TEST_SOURCE = R"(
    function main()
      var model = Model("sequential");
      <<TOKEN>>
      model.compile("scel", "adam");
    endfunction
  )";

std::string const ACTIVATION_LAYER_TEST_SOURCE = R"(
     function main() : Tensor
         var model = Model("sequential");
         model.add("activation", "<<ACTIVATION>>");
         model.compile("mse", "sgd");

         var shape = Array<UInt64>(2);
         shape[0] = 1u64;
         shape[1] = 1u64;
         var x = Tensor(shape);

         x.fromString("<<INPUT>>");

         var activated = model.predict(x);

         return activated;
     endfunction
)";

constexpr bool IGNORE_CHARGE_ESTIMATION = true;

class VMModelTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};

  void TestValidLayerAdding(std::string const &test_case_source,
                            bool               ignore_charge_estimation = false)
  {
    std::string const src =
        std::regex_replace(ADD_VALID_LAYER_TEST_SOURCE, std::regex("<<TOKEN>>"), test_case_source);
    ASSERT_TRUE(toolkit.Compile(src));
    if (ignore_charge_estimation)
    {
      ASSERT_TRUE(toolkit.Run(nullptr, ChargeAmount{0}));
    }
    else
    {
      ASSERT_TRUE(toolkit.Run());
    }
  }

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

  void TestActivation(std::string const &input, std::string const &activation,
                      std::string const &expected)
  {
    std::string src =
        std::regex_replace(ACTIVATION_LAYER_TEST_SOURCE, std::regex("<<ACTIVATION>>"), activation);
    src = std::regex_replace(src, std::regex("<<INPUT>>"), input);
    ASSERT_TRUE(toolkit.Compile(src));
    Variant res;
    ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));

    auto const gt =
        fetch::math::Tensor<fetch::vm_modules::math::DataType>::FromString(expected.c_str());

    auto const prediction = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();

    bool const passed = (prediction->GetTensor()).AllClose(gt);
    if (!passed)
    {
      std::cout << "Expected: " << gt.ToString() << std::endl;
      std::cout << " Result : " << prediction->ToString()->string() << std::endl;
    }

    EXPECT_TRUE(passed);
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
        data_shape[1] = 250u64;
        var label_shape = Array<UInt64>(2);
        label_shape[0] = 1u64;
        label_shape[1] = 250u64;
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
        data_shape[1] = 250u64;
        var label_shape = Array<UInt64>(2);
        label_shape[0] = 1u64;
        label_shape[1] = 250u64;
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
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_add_dense_noact)
{
  TestValidLayerAdding(R"(model.add("dense", 10u64, 10u64);)");
}

TEST_F(VMModelTests, model_add_dense_relu)
{
  TestValidLayerAdding(R"(model.add("dense", 10u64, 10u64, "relu");)");
}

// Disabled until implementation of AddLayerConv estimator
TEST_F(VMModelTests, model_add_conv1d_noact)
{
  TestValidLayerAdding(R"(model.add("conv1d", 10u64, 10u64, 10u64, 10u64);)",
                       IGNORE_CHARGE_ESTIMATION);
}

// Disabled until implementation of AddLayerConv estimator
TEST_F(VMModelTests, model_add_conv1d_relu)
{
  TestValidLayerAdding(R"(model.add("conv1d", 10u64, 10u64, 10u64, 10u64, "relu");)",
                       IGNORE_CHARGE_ESTIMATION);
}

// Disabled until implementation of AddLayerConv estimator
TEST_F(VMModelTests, model_add_conv2d_noact)
{
  TestValidLayerAdding(R"(model.add("conv2d", 10u64, 10u64, 10u64, 10u64);)",
                       IGNORE_CHARGE_ESTIMATION);
}

// Disabled until implementation of AddLayerConv estimator
TEST_F(VMModelTests, model_add_conv2d_relu)
{
  TestValidLayerAdding(R"(model.add("conv2d", 10u64, 10u64, 10u64, 10u64, "relu");)",
                       IGNORE_CHARGE_ESTIMATION);
}

TEST_F(VMModelTests, model_add_dropout)
{
  TestValidLayerAdding(R"(model.add("dropout", 0.256fp64);)", IGNORE_CHARGE_ESTIMATION);
}

TEST_F(VMModelTests, model_add_flatten)
{
  TestValidLayerAdding(R"(model.add("flatten");)", IGNORE_CHARGE_ESTIMATION);
}

TEST_F(VMModelTests, model_add_activation)
{
  TestValidLayerAdding(R"(model.add("activation", "relu");)", IGNORE_CHARGE_ESTIMATION);
  TestValidLayerAdding(R"(model.add("activation", "leaky_relu");)", IGNORE_CHARGE_ESTIMATION);
  TestValidLayerAdding(R"(model.add("activation", "gelu");)", IGNORE_CHARGE_ESTIMATION);
  TestValidLayerAdding(R"(model.add("activation", "sigmoid");)", IGNORE_CHARGE_ESTIMATION);
  TestValidLayerAdding(R"(model.add("activation", "log_sigmoid");)", IGNORE_CHARGE_ESTIMATION);
  TestValidLayerAdding(R"(model.add("activation", "softmax");)", IGNORE_CHARGE_ESTIMATION);
  TestValidLayerAdding(R"(model.add("activation", "log_softmax");)", IGNORE_CHARGE_ESTIMATION);
}

TEST_F(VMModelTests, model_add_invalid_layer_type)
{
  TestInvalidLayerAdding(R"(model.add("INVALID_LAYER_TYPE", 1u64, 1u64);)");
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

TEST_F(VMModelTests, model_add_activation_invalid_params)
{
  TestInvalidLayerAdding(R"(model.add("activation", "UNKNOWN_ACTIVATION");)");
}

TEST_F(VMModelTests, model_add_layers_invalid_activation_dense)
{
  TestInvalidLayerAdding(R"(model.add("dense", 10u64, 10u64, "INVALID_ACTIVATION_DENSE");)");
}

TEST_F(VMModelTests, model_add_dropout_invalid_params)
{
  TestInvalidLayerAdding(R"(model.add("dropout", 10fp64);)");
}

TEST_F(VMModelTests, model_add_reshape_invalid_params)
{
  TestInvalidLayerAdding(R"(model.add("reshape", 10fp64);)");
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

TEST_F(VMModelTests, model_uncompilable_add_layer__flatten_invalid_params)
{
  TestAddingUncompilableLayer(R"(model.add("flatten", 10fp32);)");
}

TEST_F(VMModelTests, model_uncompilable_add_layer__conv_invalid_params)
{
  TestAddingUncompilableLayer(R"(model.add("conv1d", 0u64, 10fp32, 10u64, 10u64, "relu");)");
}

TEST_F(VMModelTests, model_uncompilable_add_layer__dropout_invalid_params)
{
  TestAddingUncompilableLayer(R"(model.add("dropout", 0u64);)");
}

TEST_F(VMModelTests, model_uncompilable_add_layer__reshape_invalid_params)
{
  TestAddingUncompilableLayer(R"(model.add("reshape", 1u64);)");
}

TEST_F(VMModelTests, model_uncompilable_add_layer__activation_invalid_params)
{
  TestAddingUncompilableLayer(R"(model.add("activation", 0u64);)");
}

TEST_F(VMModelTests, model_add_layer_to_non_sequential)
{
  static char const *SRC = R"(
        function main()
          var model = Model("regressor");
          model.add("conv1d", 1u64, 1u64, 1u64, 1u64);
        endfunction
      )";
  EXPECT_TRUE(toolkit.Compile(SRC));
  std::cout << "Testing manual layer adding to a regressor model" << std::endl;
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

TEST_F(VMModelTests, DISABLED_model_compilation_simple_with_wrong_optimizer)
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

TEST_F(VMModelTests, DISABLED_model_compilation_simple_with_too_few_layer_shapes)
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

// Disableduntil AddDropout estimator implementation
TEST_F(VMModelTests, model_dropout_comparison)
{
  static char const *SOURCE = R"(
    function main()
        var dropouted_model = Model("sequential");
        dropouted_model.add("dropout", 0.1fp64);
        dropouted_model.compile("mse", "adam");

        var reference_model = Model("sequential");
        // Dropout with probability 0 acts as a simple connection
        // between input layers and output layer; this workaround is needed
        // because a sequential model with direct connection of inputs to
        // outputs can not be compiled.
        reference_model.add("dropout", 0.0fp64);
        reference_model.compile("mse", "adam");

        var shape = Array<UInt64>(3);
        shape[0] = 25u64;
        shape[1] = 25u64;
        shape[2] = 1u64;
        var x = Tensor(shape);

        x.fillRandom();

        var y = x.copy();
        y += y;

        var old_ref_loss = 0.0fp64;
        for (i in 0:5)
            dropouted_model.fit(x, y, 10u64);
            reference_model.fit(x, y, 10u64);
            var new_loss = dropouted_model.evaluate();
            var new_ref_loss = reference_model.evaluate();

            if (old_ref_loss == 0.0fp64)
              old_ref_loss = new_ref_loss[0];
            endif
            assert(new_ref_loss[0] == old_ref_loss, "Model corrupts input data!");
            assert(new_loss[0] != new_ref_loss[0], "Dropout did not change a layer output during training!");
        endfor

    endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SOURCE));
  EXPECT_TRUE(toolkit.Run(nullptr, ChargeAmount{0}));
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
  std::cout << "Testing misuse of sequential compile by passing hidden layers" << std::endl;
  EXPECT_FALSE(toolkit.Run());
}  // namespace

TEST_F(VMModelTests, dense_sequential_model_test)
{
  static char const *sequential_model_src = R"(
    function main()

      // set up data and labels
      var data_shape = Array<UInt64>(2);
      data_shape[0] = 10u64;
      data_shape[1] = 250u64;
      var label_shape = Array<UInt64>(2);
      label_shape[0] = 1u64;
      label_shape[1] = 250u64;
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
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  auto const prediction = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();

  fetch::math::Tensor<fetch::vm_modules::math::DataType> gt({5, 1});
  gt(0, 0) = fetch::math::Type<DataType>("+4.592834088");
  gt(1, 0) = fetch::math::Type<DataType>("-1.145004561");
  gt(2, 0) = fetch::math::Type<DataType>("+1.795713195");
  gt(3, 0) = fetch::math::Type<DataType>("+2.958410677");
  gt(4, 0) = fetch::math::Type<DataType>("+3.157947287");
  // the actual model output is {5, 1, 1}
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
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  auto const prediction = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();

  fetch::math::Tensor<fetch::vm_modules::math::DataType> gt({5, 1});
  gt(0, 0) = fetch::math::Type<DataType>("+3.924331061");
  gt(1, 0) = fetch::math::Type<DataType>("+6.421101891");
  gt(2, 0) = fetch::math::Type<DataType>("-0.231269899");
  gt(3, 0) = fetch::math::Type<DataType>("+7.779843630");
  gt(4, 0) = fetch::math::Type<DataType>("+10.291701029");
  // the actual model output is {5, 1, 1, 1}
  ASSERT_TRUE((prediction->GetTensor())
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
}

TEST_F(VMModelTests, DISABLED_classifier_model_test)
{
  static char const *classifier_model_src = R"(
    function main()

      // set up data and labels
      var data_shape = Array<UInt64>(2);
      data_shape[0] = 10u64;
      data_shape[1] = 250u64;
      var label_shape = Array<UInt64>(2);
      label_shape[0] = 10u64;
      label_shape[1] = 250u64;
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

TEST_F(VMModelTests, DISABLED_regressor_model_test)
{
  static char const *regressor_model_src = R"(
    function main()

      // set up data and labels
      var data_shape = Array<UInt64>(2);
      data_shape[0] = 10u64;
      data_shape[1] = 250u64;
      var label_shape = Array<UInt64>(2);
      label_shape[0] = 1u64;
      label_shape[1] = 250u64;
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

TEST_F(VMModelTests, model_with_metric)
{
  static char const *SRC_METRIC = R"(
        function main() : Array<Fixed64>
          // set up data and labels
          var data_shape = Array<UInt64>(2);
          data_shape[0] = 10u64;
          data_shape[1] = 250u64;
          var label_shape = Array<UInt64>(2);
          label_shape[0] = 1u64;
          label_shape[1] = 250u64;
          var data = Tensor(data_shape);
          var label = Tensor(label_shape);

          // set up model
          var model = Model("sequential");
          model.add("dense", 10u64, 10u64, "relu");
          model.add("dense", 10u64, 10u64, "relu");
          model.add("dense", 10u64, 1u64);
          model.compile("mse", "adam", {"mse"});

          // train the model
          model.fit(data, label, 32u64);

          // evaluate
          var mets = model.evaluate();
          return mets;
        endfunction
      )";

  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  Variant res;
  EXPECT_TRUE(toolkit.Run(&res));

  auto const metrics = res.Get<Ptr<Array<fetch::vm_modules::math::DataType>>>();
  EXPECT_EQ(metrics->elements.at(0), metrics->elements.at(1));
}

TEST_F(VMModelTests, model_with_accuracy_metric)
{
  static char const *SRC_METRIC = R"(
        function main() : Array<Fixed64>
          // set up data and labels
          var data_shape = Array<UInt64>(2);
          data_shape[0] = 10u64;
          data_shape[1] = 250u64;
          var label_shape = Array<UInt64>(2);
          label_shape[0] = 7u64;
          label_shape[1] = 250u64;
          var data = Tensor(data_shape);
          var label = Tensor(label_shape);

          // set up model
          var model = Model("sequential");
          model.add("dense", 10u64, 10u64, "relu");
          model.add("dense", 10u64, 10u64, "relu");
          model.add("dense", 10u64, 7u64);
          model.compile("scel", "adam", {"categorical accuracy"});

          // train the model
          model.fit(data, label, 32u64);

          // evaluate
          var mets = model.evaluate();
          return mets;
        endfunction
      )";

  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const metrics = res.Get<Ptr<Array<fetch::vm_modules::math::DataType>>>();
  EXPECT_GE(metrics->elements.at(1), DataType{0});
  EXPECT_LE(metrics->elements.at(1), DataType{1});
}

TEST_F(VMModelTests, model_sequential_flatten)
{
  static char const *SRC_METRIC = R"(
        function main()
          var model = Model("sequential");
          model.add("flatten");
          model.compile("scel", "adam", {"categorical accuracy"});
        endfunction
      )";

  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(nullptr, ChargeAmount{0}));
}

TEST_F(VMModelTests, model_sequential_reshape)
{
  static char const *SRC_METRIC = R"(
        function main()
          var shape = Array<UInt64>(3);
          shape[0] = 3u64;
          shape[1] = 2u64;
          shape[2] = 1u64;
          var model = Model("sequential");
          model.add("reshape", shape);
          model.compile("scel", "adam", {"categorical accuracy"});
        endfunction
      )";

  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(nullptr, ChargeAmount{0}));
}

TEST_F(VMModelTests, model_sequential_flatten_tensor_data)
{
  static char const *SRC_METRIC = R"(
        function main() : Tensor
            var shape = Array<UInt64>(1);
            shape[0] = 1u64;
            var x = Tensor(shape);
          var str_vals = "0.5, 7.1, 9.1; 6.2, 7.1, 4.; -99.1, 14328.1, 10.0;";
          x.fromString(str_vals);
          var data = x.unsqueeze();

          var model = Model("sequential");
          model.add("flatten");
          model.compile("scel", "adam", {"categorical accuracy"});
          var prediction = model.predict(data);
          print(prediction.toString());

          return prediction;
        endfunction
      )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  // we expect data columns to be sequentially concatenated
  ASSERT_EQ(stdout.str(),
            "0.500000000;"
            "6.199999999;"
            "-99.099999999;"
            "7.099999999;"
            "7.099999999;"
            "14328.099999999;"
            "9.099999999;"
            "4.000000000;"
            "10.000000000;");
  auto const tensor            = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = tensor->shape();
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({9, 1});
  EXPECT_TRUE(constructed_shape == expected.shape());
}

TEST_F(VMModelTests, model_sequential_flatten_2d_in_2d_out)
{
  static char const *SRC_METRIC = R"(
              function main() : Tensor
                 var shape = Array<UInt64>(1);
                 shape[0] = 1u64;
                 var x = Tensor(shape);
                var str_vals = "0.5, 7.1, 9.1; 6.2, 7.1, 4.;";
                x.fromString(str_vals);

                var model = Model("sequential");
                model.add("flatten");
                model.compile("scel", "adam");
                var prediction = model.predict(x);
                print(prediction.toString());

                return prediction;
              endfunction
      )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  ASSERT_EQ(stdout.str(),
            "0.500000000, 7.099999999, 9.099999999;"
            "6.199999999, 7.099999999, 4.000000000;");
  auto const tensor            = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = tensor->shape();
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({2, 3});
  EXPECT_TRUE(constructed_shape == expected.shape());
}

TEST_F(VMModelTests, model_sequential_reshape_2d_in_2d_out)
{
  static char const *SRC_METRIC = R"(
          function main() : Tensor
                          var shape = Array<UInt64>(1);
                          shape[0] = 1u64;
                          var x = Tensor(shape);
                          var str_vals = "0.5; 7.1; 9.1; 6.2;";
                          x.fromString(str_vals);

                          var to_shape = Array<UInt64>(2);
                          to_shape[0] = 4u64;
                          to_shape[1] = 1u64;

                          var model = Model("sequential");
                          model.add("reshape", to_shape);
                          model.compile("scel", "adam");
                          var prediction = model.predict(x);

                          return prediction;
          endfunction
      )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  auto const prediction        = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = prediction->shape();

  fetch::math::Tensor<fetch::vm_modules::math::DataType> gt({4, 1});
  gt(0, 0) = fetch::math::Type<DataType>("+0.5");
  gt(1, 0) = fetch::math::Type<DataType>("+7.1");
  gt(2, 0) = fetch::math::Type<DataType>("+9.1");
  gt(3, 0) = fetch::math::Type<DataType>("+6.2");
  ASSERT_TRUE((prediction->GetTensor())
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
  EXPECT_TRUE(constructed_shape == gt.shape());
}

TEST_F(VMModelTests, model_sequential_reshape_2d_in_2d_out_wrong_shape)
{
  static char const *SRC_METRIC = R"(
          function main() : Tensor
                          var shape = Array<UInt64>(1);
                          shape[0] = 1u64;
                          var x = Tensor(shape);
                          var str_vals = "0.5; 7.1; 9.1; 6.2;";
                          x.fromString(str_vals);

                          var to_shape = Array<UInt64>(2);
                          to_shape[0] = 1u64;
                          to_shape[1] = 4u64;

                          var model = Model("sequential");
                          model.add("reshape", to_shape);
                          model.compile("scel", "adam");
                          var prediction = model.predict(x);

                          return prediction;
          endfunction
      )";

  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_FALSE(toolkit.Run(nullptr, ChargeAmount{0}));
}

TEST_F(VMModelTests, model_sequential_reshape_3d_in_2d_out)
{
  static char const *SRC_METRIC = R"(
              function main() : Tensor
                var shape = Array<UInt64>(1);
                shape[0] = 1u64;
                var x = Tensor(shape);
                var str_vals = "0.5, 7.1, 9.1; 6.2, 7.1, 4.;";
                x.fromString(str_vals);
                x = x.unsqueeze();
                var to_shape = Array<UInt64>(2);
                to_shape[0] = 6u64;
                to_shape[1] = 1u64;

                var model = Model("sequential");
                model.add("reshape", to_shape);
                model.compile("scel", "adam");
                var prediction = model.predict(x);
                print(prediction.toString());

                return prediction;
              endfunction
      )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));

  auto const prediction        = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = prediction->shape();

  fetch::math::Tensor<fetch::vm_modules::math::DataType> gt({6, 1});
  gt(0, 0) = fetch::math::Type<DataType>("+0.5");
  gt(1, 0) = fetch::math::Type<DataType>("+6.2");
  gt(2, 0) = fetch::math::Type<DataType>("+7.1");
  gt(3, 0) = fetch::math::Type<DataType>("+7.1");
  gt(4, 0) = fetch::math::Type<DataType>("+9.1");
  gt(5, 0) = fetch::math::Type<DataType>("+4.0");

  ASSERT_TRUE((prediction->GetTensor())
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
  EXPECT_TRUE(constructed_shape == gt.shape());
}

TEST_F(VMModelTests, model_sequential_reshape_2d_in_3d_out)
{
  static char const *SRC_METRIC = R"(
          function main() : Tensor
                          var shape = Array<UInt64>(1);
                          shape[0] = 1u64;
                          var x = Tensor(shape);
                          var str_vals = "0.5; 7.1; 9.1; 6.2;";
                          x.fromString(str_vals);

                          var to_shape = Array<UInt64>(3);
                          to_shape[0] = 2u64;
                          to_shape[1] = 2u64;
                          to_shape[2] = 1u64;

                          var model = Model("sequential");
                          model.add("reshape", to_shape);
                          model.compile("scel", "adam");
                          var prediction = model.predict(x);

                          return prediction;
          endfunction
      )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  auto const prediction        = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = prediction->shape();

  fetch::math::Tensor<fetch::vm_modules::math::DataType> gt({2, 2});
  gt(0, 0) = fetch::math::Type<DataType>("+0.5");
  gt(1, 0) = fetch::math::Type<DataType>("+7.1");
  gt(0, 1) = fetch::math::Type<DataType>("+9.1");
  gt(1, 1) = fetch::math::Type<DataType>("+6.2");
  // the actual model output is {2, 2, 1}
  ASSERT_TRUE((prediction->GetTensor())
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({2, 2, 1});
  EXPECT_TRUE(constructed_shape == expected.shape());
}

TEST_F(VMModelTests, model_sequential_reshape_5d_in_3d_out)
{
  static char const *SRC_METRIC = R"(
          function main() : Tensor
                          var shape = Array<UInt64>(1);
                          shape[0] = 1u64;
                          var x = Tensor(shape);
                          var str_vals = "0.5; 7.1; 9.1; 6.2;";
                          x.fromString(str_vals);
                          x = x.unsqueeze();
                          x = x.unsqueeze();
                          x = x.unsqueeze();

                          var to_shape = Array<UInt64>(3);
                          to_shape[0] = 2u64;
                          to_shape[1] = 2u64;
                          to_shape[2] = 1u64;

                          var model = Model("sequential");
                          model.add("reshape", to_shape);
                          model.compile("scel", "adam");
                          var prediction = model.predict(x);

                          return prediction;
          endfunction
      )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  auto const prediction        = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = prediction->shape();

  fetch::math::Tensor<fetch::vm_modules::math::DataType> gt({2, 2});
  gt(0, 0) = fetch::math::Type<DataType>("+0.5");
  gt(1, 0) = fetch::math::Type<DataType>("+7.1");
  gt(0, 1) = fetch::math::Type<DataType>("+9.1");
  gt(1, 1) = fetch::math::Type<DataType>("+6.2");
  // the actual model output is {2, 2, 1}
  ASSERT_TRUE((prediction->GetTensor())
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({2, 2, 1});
  EXPECT_TRUE(constructed_shape == expected.shape());
}

TEST_F(VMModelTests, model_sequential_reshape_2d_in_8d_out)
{
  static char const *SRC_METRIC = R"(
          function main() : Tensor
                          var shape = Array<UInt64>(1);
                          shape[0] = 1u64;
                          var x = Tensor(shape);
                          var str_vals = "0.5; 7.1; 8.0999; 6.2;";
                          x.fromString(str_vals);

                          var to_shape = Array<UInt64>(8);
                          to_shape[0] = 2u64;
                          to_shape[1] = 2u64;
                          to_shape[2] = 1u64;
                          to_shape[3] = 1u64;
                          to_shape[4] = 1u64;
                          to_shape[5] = 1u64;
                          to_shape[6] = 1u64;
                          to_shape[7] = 1u64;

                          var model = Model("sequential");
                          model.add("reshape", to_shape);
                          model.compile("scel", "adam");
                          var prediction = model.predict(x);

                          return prediction;
          endfunction
      )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  auto const prediction        = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = prediction->shape();

  fetch::math::Tensor<fetch::vm_modules::math::DataType> gt({2, 2});
  gt(0, 0) = fetch::math::Type<DataType>("+0.5");
  gt(1, 0) = fetch::math::Type<DataType>("+7.1");
  gt(0, 1) = fetch::math::Type<DataType>("+8.0999");
  gt(1, 1) = fetch::math::Type<DataType>("+6.2");
  ASSERT_TRUE((prediction->GetTensor())
                  .AllClose(gt, fetch::math::function_tolerance<DataType>(),
                            fetch::math::function_tolerance<DataType>()));
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({2, 2, 1, 1, 1, 1, 1, 1});
  EXPECT_TRUE(constructed_shape == expected.shape());
}

TEST_F(VMModelTests, model_sequential_flatten_4d_in_2d_out)
{
  static char const *SRC_METRIC = R"(
              function main() : Tensor
                var shape = Array<UInt64>(1);
                shape[0] = 1u64;
                var x = Tensor(shape);
                var str_vals = "0.5, 7.1, 9.1; 6.2, 7.1, 4.;";
                x.fromString(str_vals);
                x = x.unsqueeze();
                x = x.unsqueeze();

                var model = Model("sequential");
                model.add("flatten");
                model.compile("scel", "adam");
                var prediction = model.predict(x);
                print(prediction.toString());

                return prediction;
              endfunction
      )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  ASSERT_EQ(stdout.str(),
            "0.500000000;"
            "6.199999999;"
            "7.099999999;"
            "7.099999999;"
            "9.099999999;"
            "4.000000000;");
  auto const tensor            = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = tensor->shape();
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({6, 1});
  EXPECT_TRUE(constructed_shape == expected.shape());
}

TEST_F(VMModelTests, model_sequential_activation_layer_relu)
{
  std::string const input = "-1000.0, -10.0, -1.0, -0.1, -0.0001; 0.0, 0.0001, 0.1, 1.0, 1000.0;";
  std::string const activation = "relu";
  std::string const result     = "0.0,   0.0,  0.0,  0.0,     0.0; 0.0, 0.0001, 0.1, 1.0, 1000.0;";

  TestActivation(input, activation, result);
}

TEST_F(VMModelTests, model_sequential_activation_layer_leaky_relu)
{
  std::string const input = "-1000.0, -10.0, -1.0, -0.1, -0.0001; 0.0, 0.0001, 0.1, 1.0, 1000.0;";
  std::string const activation = "leaky_relu";
  std::string const result =
      "-9.999999776, -0.099999997, -0.009999999, -0.001000000, -0.000001000; 0.000000000, "
      "0.000099999, 0.099999999, 1.000000000, 1000.000000000";
  TestActivation(input, activation, result);
}

TEST_F(VMModelTests, model_sequential_activation_layer_sigmoid)
{
  std::string const input = "-1000.0, -10.0, -1.0, -0.1, -0.0001; 0.0, 0.0001, 0.1, 1.0, 1000.0;";
  std::string const activation = "sigmoid";
  std::string const result =
      "0.0, 0.000045398, 0.268941422, 0.475020813, 0.499975; 0.5, 0.500025, "
      "0.524979188, 0.731058579, 0.999999999";
  TestActivation(input, activation, result);
}

TEST_F(VMModelTests, model_sequential_activation_layer_log_sigmoid)
{
  std::string const input      = "1000.0, 10.0, 1.0, 0.1, 0.0001; 0.0, 0.0001, 0.1, 1.0, 1000.0;";
  std::string const activation = "log_sigmoid";
  std::string const result =
      "0.000000000, -0.000045421, -0.313261687, -0.644396660, -0.693097181;-0.693147203, "
      "-0.693097181, -0.644396660, -0.313261687, 0.000000000;";
  TestActivation(input, activation, result);
}

TEST_F(VMModelTests, model_sequential_activation_layer_softmax)
{
  std::string const input = "-1000.0, -10.0, -1.0, -0.1, -0.0001; 0.0, 0.0001, 0.1, 1.0, 1000.0;";
  std::string const activation = "softmax";
  std::string const result =
      "0.000000000, 0.000045393, 0.249739894, 0.249739894, 0.000000000;0.999999999, 0.999954606, "
      "0.750260105, 0.750260105, 0.999999999;";
  TestActivation(input, activation, result);
}

TEST_F(VMModelTests, model_sequential_activation_layer_log_softmax)
{
  std::string const input      = "11.0, 12.0, 13.0, 14.0, 15.0; 16.0, 17.0, 18.0, 19.0, 10.0";
  std::string const activation = "log_softmax";
  std::string const result =
      "-4.451914411, -3.451914400, -2.451914397, -1.451914403, -0.451914395; -3.440269167, "
      "-2.440269163, -1.440269170, -0.440269160, -9.440271756;";
  TestActivation(input, activation, result);
}

TEST_F(VMModelTests, model_sequential_activation_layer_gelu)
{
  std::string const input      = "0.0, 1.0, 2.0, 3.0, 4.0; 0.0, -1.0, -2.0, -3.0, -4.0;";
  std::string const activation = "gelu";
  std::string const result =
      "0.000000000, 0.841191998, 1.954597482, 2.996362537, 3.999929750;0.000000000, -0.158808001, "
      "-0.045402517, -0.003637462, -0.000070249;";
  TestActivation(input, activation, result);
}

TEST_F(VMModelTests, model_sequential_flatten_1d_in_2d_out)
{
  static char const *SRC_METRIC = R"(
              function main() : Tensor
                var shape = Array<UInt64>(1);
                shape[0] = 1u64;
                var x = Tensor(shape);

                var model = Model("sequential");
                model.add("flatten");
                model.compile("scel", "adam");
                var prediction = model.predict(x);
                print(prediction.toString());

                return prediction;
              endfunction
      )";

  Variant res;
  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_TRUE(toolkit.Run(&res, ChargeAmount{0}));
  ASSERT_EQ(stdout.str(), "0.000000000;");
  auto const tensor            = res.Get<Ptr<fetch::vm_modules::math::VMTensor>>();
  auto const constructed_shape = tensor->shape();
  fetch::math::Tensor<fetch::fixed_point::fp64_t> expected({1, 1});
  EXPECT_TRUE(constructed_shape == expected.shape());
}

TEST_F(VMModelTests, model_sequential_no_layers_with_metrics)
{
  static char const *SRC_METRIC = R"(
        function main() : Array<Fixed64>
          // set up data and labels
          var data_shape = Array<UInt64>(2);
          data_shape[0] = 10u64;
          data_shape[1] = 250u64;
          var label_shape = Array<UInt64>(2);
          label_shape[0] = 7u64;
          label_shape[1] = 250u64;
          var data = Tensor(data_shape);
          var label = Tensor(label_shape);

          // set up model
          var model = Model("sequential");
          model.compile("scel", "adam", {"categorical accuracy"});

          // train the model
          model.fit(data, label, 32u64);

          // evaluate
          var mets = model.evaluate();
          return mets;
        endfunction
      )";

  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_sequential_no_layers)
{
  static char const *SEQUENTIAL_SRC = R"(
        function main()
          var model = Model("sequential");
          model.compile("mse", "adam");
        endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SEQUENTIAL_SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_sequential_multiple_compile)
{
  static char const *SEQUENTIAL_SRC = R"(
        function main()
          var model = Model("sequential");
          model.add("dense", 10u64, 10u64, "relu");
          model.compile("mse", "adam");
          model.compile("scel", "adam");
          model.compile("mse", "adam");
        endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SEQUENTIAL_SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_sequential_add_after_compile)
{
  static char const *SEQUENTIAL_SRC = R"(
      function main()
         var model = Model("sequential");
         model.add("dense", 10u64, 10u64, "relu");
         model.compile("mse", "adam");
         model.add("dense", 10u64, 1u64, "relu");
         model.compile("mse", "adam");
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SEQUENTIAL_SRC));
  EXPECT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_sequential_predict_before_fit)
{
  static char const *SEQUENTIAL_SRC = R"(
      function main() : Array<Fixed64>
        // set up data and labels
        var data_shape = Array<UInt64>(2);
        data_shape[0] = 10u64;
        data_shape[1] = 250u64;
        var label_shape = Array<UInt64>(2);
        label_shape[0] = 7u64;
        label_shape[1] = 250u64;
        var data = Tensor(data_shape);
        var label = Tensor(label_shape);

        // set up model
        var model = Model("sequential");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 7u64);
        model.compile("scel", "adam", {"categorical accuracy"});

        var prediction = model.predict(data);

        // train the model
        model.fit(data, label, 32u64);

        // evaluate performance
        var mets = model.evaluate();

        return mets;
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SEQUENTIAL_SRC));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(VMModelTests, model_sequential_predict_bad_data)
{
  static char const *SEQUENTIAL_SRC = R"(
      function main()
        // set up malformed data tensor with shape 0,0
        var data_shape = Array<UInt64>(2);
        var data = Tensor(data_shape);

        // set up model
        var model = Model("sequential");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 7u64);
        model.compile("mse", "adam", {"categorical accuracy"});

        var prediction = model.predict(data);
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SEQUENTIAL_SRC));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_sequential_evaluate_without_fit)
{
  static char const *SEQUENTIAL_SRC = R"(
      function main()
        // set up model
        var model = Model("sequential");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 7u64);
        model.compile("mse", "adam", {"categorical accuracy"});

        var prediction = model.evaluate();
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SEQUENTIAL_SRC));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_sequential_fit_bad_data)
{
  static char const *SEQUENTIAL_SRC = R"(
      function main()
        // set up data and labels
        var data_shape = Array<UInt64>(2);
        var label_shape = Array<UInt64>(2);
        var data = Tensor(data_shape);
        var label = Tensor(label_shape);

        // set up model
        var model = Model("sequential");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 10u64, "relu");
        model.add("dense", 10u64, 7u64);
        model.compile("scel", "adam", {"categorical accuracy"});

        model.fit(data, label, 32u64);
      endfunction
    )";

  ASSERT_TRUE(toolkit.Compile(SEQUENTIAL_SRC));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(VMModelTests, model_fit_and_refit)
{
  static char const *SRC_METRIC = R"(
        function main()
          // set up data and labels
          var data_shape = Array<UInt64>(2);
          data_shape[0] = 10u64;
          data_shape[1] = 250u64;
          var label_shape = Array<UInt64>(2);
          label_shape[0] = 7u64;
          label_shape[1] = 250u64;
          var data = Tensor(data_shape);
          var label = Tensor(label_shape);

          // set up model
          var model = Model("sequential");
          model.add("dense", 10u64, 10u64, "relu");
          model.add("dense", 10u64, 7u64);
          model.compile("scel", "adam");

          // train the model
          model.fit(data, label, 32u64);

          // new data and labels
          var data_shape2 = Array<UInt64>(2);
          data_shape2[0] = 10u64;
          data_shape2[1] = 123u64;
          var label_shape2 = Array<UInt64>(2);
          label_shape2[0] = 7u64;
          label_shape2[1] = 123u64;
          var data2 = Tensor(data_shape2);
          var label2 = Tensor(label_shape2);

          // train the model again
          model.fit(data2, label2, 16u64);

        endfunction
      )";

  ASSERT_TRUE(toolkit.Compile(SRC_METRIC));

  ASSERT_TRUE(toolkit.Run());
}

}  // namespace
