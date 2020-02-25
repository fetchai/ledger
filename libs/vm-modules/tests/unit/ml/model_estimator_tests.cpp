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

#include "core/serializers/main_serializer.hpp"
#include "ml/layers/fully_connected.hpp"
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/vm_factory.hpp"

#include "gmock/gmock.h"

namespace {

using SizeType     = fetch::math::SizeType;
using DataType     = fetch::vm_modules::math::DataType;
using VmStringPtr  = fetch::vm::Ptr<fetch::vm::String>;
using VmModel      = fetch::vm_modules::ml::model::VMModel;
using VmModelPtr   = fetch::vm::Ptr<VmModel>;
using ChargeAmount = fetch::vm::ChargeAmount;
using VmTensorPtr  = fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>;
using VMPtr        = std::shared_ptr<fetch::vm::VM>;

////////////////////////
// VM objects helpers //
////////////////////////

fetch::vm::Ptr<fetch::vm::String> VmString(VMPtr &vm, std::string const &str)
{
  return fetch::vm::Ptr<fetch::vm::String>{new fetch::vm::String{vm.get(), str}};
}

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> VmTensor(
    VMPtr &vm, std::vector<fetch::math::SizeType> const &shape)
{
  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(shape);
}

fetch::vm::Ptr<fetch::vm::Array<uint64_t>> VmArray(std::shared_ptr<fetch::vm::VM> &vm,
                                                   std::vector<uint64_t> const &   values)
{
  std::size_t                                size = values.size();
  fetch::vm::Ptr<fetch::vm::Array<uint64_t>> array =
      vm->CreateNewObject<fetch::vm::Array<uint64_t>>(vm->GetTypeId<uint64_t>(),
                                                      static_cast<int32_t>(size));

  for (std::size_t i{0}; i < size; ++i)
  {
    array->elements[i] = values[i];
  }

  return array;
}

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> VmSequentialModel(VMPtr &vm)
{
  auto model_category = VmString(vm, "sequential");
  auto model          = vm->CreateNewObject<fetch::vm_modules::ml::model::VMModel>(model_category);
  return model;
}

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> VmSequentialModel(
    VMPtr &vm, std::vector<SizeType> &sizes, std::vector<bool> &activations)
{
  if (sizes.size() != (activations.size() + 1))
  {
    throw std::runtime_error{"Wrong configuration for multilayer VMModel"};
  }

  auto model           = VmSequentialModel(vm);
  auto size            = activations.size();
  auto layer_type      = VmString(vm, "dense");
  auto activation_type = VmString(vm, "relu");

  for (std::size_t i{0}; i < size; ++i)
  {
    auto input_size  = sizes[i];
    auto output_size = sizes[i + 1];

    if (activations[i])
    {
      model->EstimateLayerAddDenseActivation(layer_type, input_size, output_size, activation_type);
      model->LayerAddDenseActivation(layer_type, input_size, output_size, activation_type);
    }
    else
    {
      model->EstimateLayerAddDense(layer_type, input_size, output_size);
      model->LayerAddDense(layer_type, input_size, output_size);
    }
  }

  return model;
}

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> VmSequentialModel(
    VMPtr &vm, std::vector<SizeType> &sizes, std::vector<bool> &activations,
    std::string const &loss, std::string const &optimiser)
{
  auto model = VmSequentialModel(vm, sizes, activations);

  // compile model
  auto vm_loss      = VmString(vm, loss);
  auto vm_optimiser = VmString(vm, optimiser);
  model->EstimateCompileSequential(vm_loss, vm_optimiser);
  model->CompileSequential(vm_loss, vm_optimiser);

  return model;
}

/////////////
// Fixture //
/////////////

class VMModelEstimatorTests : public ::testing::Test
{
public:
  VMPtr vm;

  void SetUp() override
  {
    // setup the VM
    using VMFactory = fetch::vm_modules::VMFactory;
    auto module     = VMFactory::GetModule(fetch::vm_modules::VMFactory::USE_ALL);
    vm              = std::make_shared<fetch::vm::VM>(module.get());
  }

  ChargeAmount LayerAddDenseCharge(VmModelPtr &model, VmStringPtr const &layer_type,
                                   SizeType input_size, SizeType output_size)
  {
    return model->EstimateLayerAddDense(layer_type, input_size, output_size);
  }

  ChargeAmount LayerAddDenseActivationCharge(VmModelPtr &model, VmStringPtr const &layer_type,
                                             SizeType input_size, SizeType output_size,
                                             VmStringPtr const &activation)
  {
    return model->EstimateLayerAddDenseActivation(layer_type, input_size, output_size, activation);
  }

  ChargeAmount CompileSequentialCharge(VmModelPtr &model, VmStringPtr const &loss,
                                       VmStringPtr const &optimiser)
  {
    return model->EstimateCompileSequential(loss, optimiser);
  }

  ChargeAmount FitCharge(VmModelPtr &model, VmTensorPtr const &data, VmTensorPtr const &label,
                         SizeType batch_size)
  {
    return model->EstimateFit(data, label, batch_size);
  }

  ChargeAmount PredictCharge(VmModelPtr &model, VmTensorPtr const &data)
  {
    return model->EstimatePredict(data);
  }

  ChargeAmount SerializeToStringCharge(VmModelPtr &model)
  {
    return model->EstimateSerializeToString();
  }

  ChargeAmount DeserializeFromStringCharge(VmModelPtr &model, VmStringPtr const &model_serialized)
  {
    return model->EstimateDeserializeFromString(model_serialized);
  }
};

/////////////////////////////////////////////////////
// sanity check that estimator behaves as intended //
/////////////////////////////////////////////////////

TEST_F(VMModelEstimatorTests, add_dense_layer_test)
{
  std::string model_type = "sequential";
  std::string layer_type = "dense";

  SizeType min_input_size  = 0;
  SizeType max_input_size  = 1000;
  SizeType input_step      = 10;
  SizeType min_output_size = 0;
  SizeType max_output_size = 1000;
  SizeType output_step     = 10;

  VmStringPtr       vm_ptr_layer_type{new fetch::vm::String(vm.get(), layer_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(vm.get(), type_id, model_type);

  for (SizeType inputs = min_input_size; inputs < max_input_size; inputs += input_step)
  {
    for (SizeType outputs = min_output_size; outputs < max_output_size; outputs += output_step)
    {
      SizeType val =
          fetch::ml::layers::FullyConnected<fetch::math::Tensor<DataType>>::ChargeConstruct(
              inputs, outputs);

      EXPECT_TRUE(model.EstimateLayerAddDense(vm_ptr_layer_type, inputs, outputs) ==
                  static_cast<ChargeAmount>(val) + 1);
    }
  }
}

TEST_F(VMModelEstimatorTests, add_dense_layer_activation_test)
{
  std::string                        model_type          = "sequential";
  std::string                        layer_type          = "dense";
  std::string                        activation_type_str = "relu";
  fetch::ml::details::ActivationType activation_type     = fetch::ml::details::ActivationType::RELU;

  SizeType min_input_size  = 0;
  SizeType max_input_size  = 1000;
  SizeType input_step      = 10;
  SizeType min_output_size = 0;
  SizeType max_output_size = 1000;
  SizeType output_step     = 10;

  VmStringPtr       vm_ptr_layer_type{new fetch::vm::String(vm.get(), layer_type)};
  VmStringPtr       vm_ptr_activation_type{new fetch::vm::String(vm.get(), activation_type_str)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(vm.get(), type_id, model_type);

  for (SizeType inputs = min_input_size; inputs < max_input_size; inputs += input_step)
  {
    for (SizeType outputs = min_output_size; outputs < max_output_size; outputs += output_step)
    {

      SizeType val =
          fetch::ml::layers::FullyConnected<fetch::math::Tensor<DataType>>::ChargeConstruct(
              inputs, outputs, activation_type);

      EXPECT_TRUE(model.EstimateLayerAddDenseActivation(vm_ptr_layer_type, inputs, outputs,
                                                        vm_ptr_activation_type) ==
                  static_cast<ChargeAmount>(val) + 1);
    }
  }
}

TEST_F(VMModelEstimatorTests, add_conv_layer_test)
{
  std::string model_type = "sequential";
  std::string layer_type = "convolution1D";

  SizeType min_input_size = 0;
  SizeType max_input_size = 500;
  SizeType input_step     = 10;

  SizeType min_output_size = 0;
  SizeType max_output_size = 500;
  SizeType output_step     = 10;

  SizeType min_kernel_size = 0;
  SizeType max_kernel_size = 100;
  SizeType kernel_step     = 10;

  SizeType min_stride_size = 0;
  SizeType max_stride_size = 100;
  SizeType stride_step     = 10;

  VmStringPtr       vm_ptr_layer_type{new fetch::vm::String(vm.get(), layer_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(vm.get(), type_id, model_type);

  for (SizeType output_channels = min_output_size; output_channels < max_output_size;
       output_channels += output_step)
  {
    for (SizeType input_channels = min_input_size; input_channels < max_input_size;
         input_channels += input_step)
    {
      for (SizeType kernel_size = min_kernel_size; kernel_size < max_kernel_size;
           kernel_size += kernel_step)
      {
        for (SizeType stride_size = min_stride_size; stride_size < max_stride_size;
             stride_size += stride_step)
        {

          EXPECT_TRUE(model.EstimateLayerAddConv(vm_ptr_layer_type, output_channels, input_channels,
                                                 kernel_size, stride_size) ==
                      static_cast<ChargeAmount>(fetch::vm::MAXIMUM_CHARGE));
        }
      }
    }
  }
}

TEST_F(VMModelEstimatorTests, add_conv_layer_activation_test)
{
  std::string model_type      = "sequential";
  std::string layer_type      = "convolution1D";
  std::string activation_type = "relu";

  SizeType min_input_size = 0;
  SizeType max_input_size = 500;
  SizeType input_step     = 10;

  SizeType min_output_size = 0;
  SizeType max_output_size = 500;
  SizeType output_step     = 10;

  SizeType min_kernel_size = 0;
  SizeType max_kernel_size = 100;
  SizeType kernel_step     = 10;

  SizeType min_stride_size = 0;
  SizeType max_stride_size = 100;
  SizeType stride_step     = 10;

  VmStringPtr       vm_ptr_layer_type{new fetch::vm::String(vm.get(), layer_type)};
  VmStringPtr       vm_ptr_activation_type{new fetch::vm::String(vm.get(), activation_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(vm.get(), type_id, model_type);

  for (SizeType output_channels = min_output_size; output_channels < max_output_size;
       output_channels += output_step)
  {
    for (SizeType input_channels = min_input_size; input_channels < max_input_size;
         input_channels += input_step)
    {
      for (SizeType kernel_size = min_kernel_size; kernel_size < max_kernel_size;
           kernel_size += kernel_step)
      {
        for (SizeType stride_size = min_stride_size; stride_size < max_stride_size;
             stride_size += stride_step)
        {

          EXPECT_TRUE(model.EstimateLayerAddConvActivation(vm_ptr_layer_type, output_channels,
                                                           input_channels, kernel_size, stride_size,
                                                           vm_ptr_activation_type) ==
                      static_cast<ChargeAmount>(fetch::vm::MAXIMUM_CHARGE));
        }
      }
    }
  }
}

TEST_F(VMModelEstimatorTests, compile_sequential_test)
{
  std::string model_type = "sequential";
  std::string layer_type = "dense";
  std::string loss_type  = "mse";
  std::string opt_type   = "adam";

  SizeType min_input_size  = 1;
  SizeType max_input_size  = 1001;
  SizeType input_step      = 100;
  SizeType min_output_size = 1;
  SizeType max_output_size = 1001;
  SizeType output_step     = 100;

  fetch::vm::TypeId type_id = 0;

  VmStringPtr vm_ptr_layer_type{new fetch::vm::String(vm.get(), layer_type)};
  VmStringPtr vm_ptr_loss_type{new fetch::vm::String(vm.get(), loss_type)};
  VmStringPtr vm_ptr_opt_type{new fetch::vm::String(vm.get(), opt_type)};

  for (SizeType inputs = min_input_size; inputs <= max_input_size; inputs += input_step)
  {
    for (SizeType outputs = min_output_size; outputs <= max_output_size; outputs += output_step)
    {
      VmModel model(vm.get(), type_id, model_type);

      // add some layers
      model.LayerAddDense(vm_ptr_layer_type, inputs, 10);
      model.LayerAddDense(vm_ptr_layer_type, 10, outputs);

      // FullyConnected1
      SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape({10, inputs});
      padded_size += fetch::math::Tensor<DataType>::PaddedSizeFromShape({10, 1});

      // FullyConnected2
      padded_size += fetch::math::Tensor<DataType>::PaddedSizeFromShape({outputs, 10});
      padded_size += fetch::math::Tensor<DataType>::PaddedSizeFromShape({outputs, 1});

      SizeType val = padded_size * 5 + 17;

      EXPECT_EQ(model.EstimateCompileSequential(vm_ptr_loss_type, vm_ptr_opt_type),
                static_cast<ChargeAmount>(val));
    }
  }
}

TEST_F(VMModelEstimatorTests, estimator_fit_and_predict_test)
{
  std::string model_type      = "sequential";
  std::string layer_type      = "dense";
  std::string loss_type       = "mse";
  std::string opt_type        = "adam";
  std::string activation_type = "relu";

  SizeType min_data_size_1  = 10;
  SizeType max_data_size_1  = 80;
  SizeType data_size_1_step = 19;

  SizeType min_data_points  = 10;
  SizeType max_data_points  = 80;
  SizeType data_points_step = 13;

  SizeType min_label_size_1  = 1;
  SizeType max_label_size_1  = 80;
  SizeType label_size_1_step = 17;

  SizeType min_batch_size  = 1;
  SizeType batch_size_step = 23;

  fetch::vm::TypeId type_id = 0;
  VmStringPtr       vm_ptr_layer_type{new fetch::vm::String(vm.get(), layer_type)};
  VmStringPtr       vm_ptr_loss_type{new fetch::vm::String(vm.get(), loss_type)};
  VmStringPtr       vm_ptr_opt_type{new fetch::vm::String(vm.get(), opt_type)};
  VmStringPtr       vm_ptr_activation_type{new fetch::vm::String(vm.get(), activation_type)};

  for (SizeType data_size_1 = min_data_size_1; data_size_1 < max_data_size_1;
       data_size_1 += data_size_1_step)
  {
    for (SizeType n_data = min_data_points; n_data < max_data_points; n_data += data_points_step)
    {
      for (SizeType label_size_1 = min_label_size_1; label_size_1 < max_label_size_1;
           label_size_1 += label_size_1_step)
      {
        for (SizeType batch_size = min_batch_size; batch_size < n_data;
             batch_size += batch_size_step)
        {

          std::vector<uint64_t>                             data_shape{{data_size_1, n_data}};
          std::vector<uint64_t>                             label_shape{{label_size_1, n_data}};
          fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> vm_ptr_tensor_data{
              new fetch::vm_modules::math::VMTensor(vm.get(), type_id, data_shape)};
          fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> vm_ptr_tensor_labels{
              new fetch::vm_modules::math::VMTensor(vm.get(), type_id, label_shape)};

          VmModel model(vm.get(), type_id, model_type);

          model.LayerAddDenseActivation(vm_ptr_layer_type, data_size_1, label_size_1,
                                        vm_ptr_activation_type);

          model.CompileSequential(vm_ptr_loss_type, vm_ptr_opt_type);

          SizeType val{0};

          SizeType subset_size       = data_shape.at(data_shape.size() - 1);
          SizeType number_of_batches = subset_size / batch_size;

          // Forward pass
          val += model.ChargeForward(data_shape) * number_of_batches;

          // Backward pass
          val += model.ChargeBackward() * subset_size;

          // FullyConnected weight sizes
          SizeType weights_size =
              fetch::math::Tensor<DataType>::SizeFromShape({label_size_1, data_size_1});
          weights_size += fetch::math::Tensor<DataType>::SizeFromShape({label_size_1, 1});

          // Optimiser step - Adam
          val += (8 + weights_size * 15) * number_of_batches;

          EXPECT_EQ(model.EstimateFit(vm_ptr_tensor_data, vm_ptr_tensor_labels, batch_size),
                    static_cast<SizeType>(val) + 1);

          ChargeAmount const predict_val = model.ChargeForward(data_shape);

          EXPECT_EQ(model.EstimatePredict(vm_ptr_tensor_data), static_cast<SizeType>(predict_val));
        }
      }
    }
  }
}

TEST_F(VMModelEstimatorTests, estimator_evaluate_with_metrics)
{
  using fetch::vm::Array;
  using fetch::vm::Ptr;
  using fetch::vm::String;

  std::string model_type      = "sequential";
  std::string layer_type      = "dense";
  std::string loss_type       = "mse";
  std::string opt_type        = "adam";
  std::string activation_type = "relu";

  SizeType min_data_size_1  = 10;
  SizeType max_data_size_1  = 80;
  SizeType data_size_1_step = 19;

  SizeType min_data_points  = 10;
  SizeType max_data_points  = 80;
  SizeType data_points_step = 13;

  SizeType min_label_size_1  = 1;
  SizeType max_label_size_1  = 80;
  SizeType label_size_1_step = 17;

  SizeType min_batch_size  = 1;
  SizeType batch_size_step = 23;

  fetch::vm::TypeId type_id = 0;
  VmStringPtr       vm_ptr_layer_type{new fetch::vm::String(vm.get(), layer_type)};
  VmStringPtr       vm_ptr_loss_type{new fetch::vm::String(vm.get(), loss_type)};
  VmStringPtr       vm_ptr_opt_type{new fetch::vm::String(vm.get(), opt_type)};
  VmStringPtr       vm_ptr_activation_type{new fetch::vm::String(vm.get(), activation_type)};

  std::vector<std::string> mets      = {"categorical accuracy", "mse", "cel", "scel"};
  SizeType                 n_metrics = mets.size();

  Ptr<Array<Ptr<String>>> metrics = Ptr<Array<Ptr<String>>>(
      new Array<Ptr<String>>(vm.get(), vm->GetTypeId<fetch::vm::IArray>(), vm->GetTypeId<String>(),
                             static_cast<int32_t>(n_metrics)));

  for (SizeType i{0}; i < n_metrics; i++)
  {
    metrics->elements.at(i) = VmStringPtr(new fetch::vm::String(vm.get(), mets.at(i)));
  }

  for (SizeType data_size_1 = min_data_size_1; data_size_1 < max_data_size_1;
       data_size_1 += data_size_1_step)
  {
    for (SizeType n_data = min_data_points; n_data < max_data_points; n_data += data_points_step)
    {
      for (SizeType label_size_1 = min_label_size_1; label_size_1 < max_label_size_1;
           label_size_1 += label_size_1_step)
      {
        for (SizeType batch_size = min_batch_size; batch_size < n_data;
             batch_size += batch_size_step)
        {
          std::vector<uint64_t>                             data_shape{{data_size_1, n_data}};
          std::vector<uint64_t>                             label_shape{{label_size_1, n_data}};
          fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> vm_ptr_tensor_data{
              new fetch::vm_modules::math::VMTensor(vm.get(), type_id, data_shape)};
          fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> vm_ptr_tensor_labels{
              new fetch::vm_modules::math::VMTensor(vm.get(), type_id, label_shape)};

          VmModel model(vm.get(), type_id, model_type);

          auto input_name  = VmString(vm, "input");
          auto input_array = VmArray(vm, {data_size_1, batch_size});

          model.LayerAddInput(input_name, input_array);

          model.LayerAddDenseActivation(vm_ptr_layer_type, data_size_1, label_size_1,
                                        vm_ptr_activation_type);

          model.CompileSequentialWithMetrics(vm_ptr_loss_type, vm_ptr_opt_type, metrics);

          // Calling Fit is needed to set the data
          model.Fit(vm_ptr_tensor_data, vm_ptr_tensor_labels, batch_size);

          ChargeAmount const cost = model.ChargeForward(data_shape);

          EXPECT_EQ(model.EstimateEvaluate(), static_cast<ChargeAmount>(cost));
        }
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Operations estimators consistency relative to their args and object state //
///////////////////////////////////////////////////////////////////////////////

TEST_F(VMModelEstimatorTests, compilesequential_charge_correlate_with_number_of_layers)
{
  auto loss      = VmString(vm, "mse");
  auto optimiser = VmString(vm, "adam");

  std::vector<SizeType> sizes_small       = {10, 100};
  std::vector<bool>     activations_small = {true};
  auto                  model_small       = VmSequentialModel(vm, sizes_small, activations_small);
  auto                  charge_small      = CompileSequentialCharge(model_small, loss, optimiser);

  std::vector<SizeType> sizes_big       = {10, 100, 10};
  std::vector<bool>     activations_big = {true, true};
  auto                  model_big       = VmSequentialModel(vm, sizes_big, activations_big);
  auto                  charge_big      = CompileSequentialCharge(model_big, loss, optimiser);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, compilesequential_charge_correlate_with_size_of_layers)
{
  auto              loss        = VmString(vm, "mse");
  auto              optimiser   = VmString(vm, "adam");
  std::vector<bool> activations = {true, true};

  std::vector<SizeType> sizes_small  = {10, 10, 10};
  auto                  model_small  = VmSequentialModel(vm, sizes_small, activations);
  auto                  charge_small = CompileSequentialCharge(model_small, loss, optimiser);

  std::vector<SizeType> sizes_big  = {10, 10, 100};
  auto                  model_big  = VmSequentialModel(vm, sizes_big, activations);
  auto                  charge_big = CompileSequentialCharge(model_big, loss, optimiser);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, fit_charge_correlate_with_number_of_layers)
{
  auto     loss       = std::string("mse");
  auto     optimiser  = std::string("adam");
  SizeType datapoints = 128;
  SizeType batch_size = 32;

  std::vector<SizeType> sizes_small       = {10, 10};
  std::vector<bool>     activations_small = {true};
  auto model_small = VmSequentialModel(vm, sizes_small, activations_small, loss, optimiser);
  std::vector<SizeType> data_shape_small{sizes_small[0], datapoints};
  std::vector<SizeType> label_shape_small{sizes_small[sizes_small.size() - 1], datapoints};
  auto                  data_small   = VmTensor(vm, data_shape_small);
  auto                  label_small  = VmTensor(vm, label_shape_small);
  auto                  charge_small = FitCharge(model_small, data_small, label_small, batch_size);

  std::vector<SizeType> sizes_big       = {10, 10, 10};
  std::vector<bool>     activations_big = {true, true};
  auto model_big = VmSequentialModel(vm, sizes_big, activations_big, loss, optimiser);
  std::vector<SizeType> data_shape_big{sizes_big[0], datapoints};
  std::vector<SizeType> label_shape_big{sizes_big[sizes_big.size() - 1], datapoints};
  auto                  data_big   = VmTensor(vm, data_shape_big);
  auto                  label_big  = VmTensor(vm, label_shape_big);
  auto                  charge_big = FitCharge(model_big, data_big, label_big, batch_size);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, fit_charge_correlate_with_size_of_layers)
{
  auto              loss        = std::string("mse");
  auto              optimiser   = std::string("adam");
  SizeType          datapoints  = 128;
  SizeType          batch_size  = 32;
  std::vector<bool> activations = {true, true};

  std::vector<SizeType> sizes_small = {10, 10, 10};
  auto model_small = VmSequentialModel(vm, sizes_small, activations, loss, optimiser);
  std::vector<SizeType> data_shape_small{sizes_small[0], datapoints};
  std::vector<SizeType> label_shape_small{sizes_small[sizes_small.size() - 1], datapoints};
  auto                  data_small   = VmTensor(vm, data_shape_small);
  auto                  label_small  = VmTensor(vm, label_shape_small);
  auto                  charge_small = FitCharge(model_small, data_small, label_small, batch_size);

  std::vector<SizeType> sizes_big = {10, 10, 100};
  auto                  model_big = VmSequentialModel(vm, sizes_big, activations, loss, optimiser);
  std::vector<SizeType> data_shape_big{sizes_big[0], datapoints};
  std::vector<SizeType> label_shape_big{sizes_big[sizes_big.size() - 1], datapoints};
  auto                  data_big   = VmTensor(vm, data_shape_big);
  auto                  label_big  = VmTensor(vm, label_shape_big);
  auto                  charge_big = FitCharge(model_big, data_big, label_big, batch_size);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, fit_charge_correlate_with_number_of_datapoints)
{
  auto                  loss        = std::string("mse");
  auto                  optimiser   = std::string("adam");
  std::vector<bool>     activations = {true, true};
  std::vector<SizeType> sizes       = {10, 100, 10};
  auto                  model       = VmSequentialModel(vm, sizes, activations, loss, optimiser);
  SizeType              batch_size  = 32;

  SizeType              datapoints_small = 32;
  std::vector<SizeType> data_shape_small{sizes[0], datapoints_small};
  std::vector<SizeType> label_shape_small{sizes[sizes.size() - 1], datapoints_small};
  auto                  data_small   = VmTensor(vm, data_shape_small);
  auto                  label_small  = VmTensor(vm, label_shape_small);
  auto                  charge_small = FitCharge(model, data_small, label_small, batch_size);

  SizeType              datapoints_big = 128;
  std::vector<SizeType> data_shape_big{sizes[0], datapoints_big};
  std::vector<SizeType> label_shape_big{sizes[sizes.size() - 1], datapoints_big};
  auto                  data_big   = VmTensor(vm, data_shape_big);
  auto                  label_big  = VmTensor(vm, label_shape_big);
  auto                  charge_big = FitCharge(model, data_big, label_big, batch_size);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, fit_charge_correlate_negatively_with_batchsize)
{
  auto                  loss        = std::string("mse");
  auto                  optimiser   = std::string("adam");
  std::vector<bool>     activations = {true, true};
  std::vector<SizeType> sizes       = {10, 100, 10};
  auto                  model       = VmSequentialModel(vm, sizes, activations, loss, optimiser);
  SizeType              datapoints  = 128;
  std::vector<SizeType> data_shape{sizes[0], datapoints};
  std::vector<SizeType> label_shape{sizes[sizes.size() - 1], datapoints};
  auto                  data  = VmTensor(vm, data_shape);
  auto                  label = VmTensor(vm, label_shape);

  SizeType batch_size_big = 64;
  auto     charge_small   = FitCharge(model, data, label, batch_size_big);

  SizeType batch_size_small = 32;
  auto     charge_big       = FitCharge(model, data, label, batch_size_small);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, predict_charge_correlate_with_number_of_layers)
{
  auto     loss       = std::string("mse");
  auto     optimiser  = std::string("adam");
  SizeType datapoints = 128;

  std::vector<SizeType> sizes_small       = {10, 10};
  std::vector<bool>     activations_small = {true};
  auto model_small = VmSequentialModel(vm, sizes_small, activations_small, loss, optimiser);
  std::vector<SizeType> data_shape_small{sizes_small[0], datapoints};
  auto                  data_small   = VmTensor(vm, data_shape_small);
  auto                  charge_small = PredictCharge(model_small, data_small);

  std::vector<SizeType> sizes_big       = {10, 10, 10};
  std::vector<bool>     activations_big = {true, true};
  auto model_big = VmSequentialModel(vm, sizes_big, activations_big, loss, optimiser);
  std::vector<SizeType> data_shape_big{sizes_big[0], datapoints};
  auto                  data_big   = VmTensor(vm, data_shape_big);
  auto                  charge_big = PredictCharge(model_big, data_big);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, predict_charge_correlate_with_size_of_layers)
{
  auto              loss        = std::string("mse");
  auto              optimiser   = std::string("adam");
  SizeType          datapoints  = 128;
  std::vector<bool> activations = {true, true};

  std::vector<SizeType> sizes_small = {10, 10, 10};
  auto model_small = VmSequentialModel(vm, sizes_small, activations, loss, optimiser);
  std::vector<SizeType> data_shape_small{sizes_small[0], datapoints};
  auto                  data_small   = VmTensor(vm, data_shape_small);
  auto                  charge_small = PredictCharge(model_small, data_small);

  std::vector<SizeType> sizes_big = {10, 100, 10};
  auto                  model_big = VmSequentialModel(vm, sizes_big, activations, loss, optimiser);
  std::vector<SizeType> data_shape_big{sizes_big[0], datapoints};
  auto                  data_big   = VmTensor(vm, data_shape_big);
  auto                  charge_big = PredictCharge(model_big, data_big);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, predict_charge_correlate_with_number_of_datapoints)
{
  auto                  loss        = std::string("mse");
  auto                  optimiser   = std::string("adam");
  std::vector<bool>     activations = {true, true};
  std::vector<SizeType> sizes       = {10, 100, 10};
  auto                  model       = VmSequentialModel(vm, sizes, activations, loss, optimiser);

  SizeType              datapoints_small = 32;
  std::vector<SizeType> data_shape_small{sizes[0], datapoints_small};
  auto                  data_small   = VmTensor(vm, data_shape_small);
  auto                  charge_small = PredictCharge(model, data_small);

  SizeType              datapoints_big = 128;
  std::vector<SizeType> data_shape_big{sizes[0], datapoints_big};
  auto                  data_big   = VmTensor(vm, data_shape_big);
  auto                  charge_big = PredictCharge(model, data_big);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, serializetostring_charge_correlate_with_number_of_layers)
{
  auto loss      = std::string("mse");
  auto optimiser = std::string("adam");

  std::vector<SizeType> sizes_small       = {10, 10};
  std::vector<bool>     activations_small = {true};
  auto model_small  = VmSequentialModel(vm, sizes_small, activations_small, loss, optimiser);
  auto charge_small = SerializeToStringCharge(model_small);

  std::vector<SizeType> sizes_big       = {10, 10, 10};
  std::vector<bool>     activations_big = {true, true};
  auto model_big  = VmSequentialModel(vm, sizes_big, activations_big, loss, optimiser);
  auto charge_big = SerializeToStringCharge(model_big);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, serializetostring_charge_correlate_with_size_of_layers)
{
  auto              loss        = std::string("mse");
  auto              optimiser   = std::string("adam");
  std::vector<bool> activations = {true, true};

  std::vector<SizeType> sizes_small = {10, 10, 10};
  auto model_small  = VmSequentialModel(vm, sizes_small, activations, loss, optimiser);
  auto charge_small = SerializeToStringCharge(model_small);

  std::vector<SizeType> sizes_big  = {10, 100, 10};
  auto                  model_big  = VmSequentialModel(vm, sizes_big, activations, loss, optimiser);
  auto                  charge_big = SerializeToStringCharge(model_big);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, deserializefromstring_charge_correlate_with_number_of_layers)
{
  auto loss      = std::string("mse");
  auto optimiser = std::string("adam");

  std::vector<SizeType> sizes_small       = {10, 10};
  std::vector<bool>     activations_small = {true};
  auto model_small = VmSequentialModel(vm, sizes_small, activations_small, loss, optimiser);
  auto model_small_serialized = model_small->SerializeToString();
  auto model_small_new        = VmSequentialModel(vm);
  auto charge_small = DeserializeFromStringCharge(model_small_new, model_small_serialized);

  std::vector<SizeType> sizes_big       = {10, 10, 10};
  std::vector<bool>     activations_big = {true, true};
  auto model_big            = VmSequentialModel(vm, sizes_big, activations_big, loss, optimiser);
  auto model_big_serialized = model_big->SerializeToString();
  auto model_big_new        = VmSequentialModel(vm);
  auto charge_big           = DeserializeFromStringCharge(model_big_new, model_big_serialized);

  EXPECT_LT(charge_small, charge_big);
}

TEST_F(VMModelEstimatorTests, deserializefromstring_charge_correlate_with_size_of_layers)
{
  auto              loss        = std::string("mse");
  auto              optimiser   = std::string("adam");
  std::vector<bool> activations = {true, true};

  std::vector<SizeType> sizes_small = {10, 10, 10};
  auto model_small            = VmSequentialModel(vm, sizes_small, activations, loss, optimiser);
  auto model_small_serialized = model_small->SerializeToString();
  auto model_small_new        = VmSequentialModel(vm);
  auto charge_small = DeserializeFromStringCharge(model_small_new, model_small_serialized);

  std::vector<SizeType> sizes_big = {10, 100, 10};
  auto                  model_big = VmSequentialModel(vm, sizes_big, activations, loss, optimiser);
  auto                  model_big_serialized = model_big->SerializeToString();
  auto                  model_big_new        = VmSequentialModel(vm);
  auto charge_big = DeserializeFromStringCharge(model_big_new, model_big_serialized);

  EXPECT_LT(charge_small, charge_big);
}

//////////////////////////////////////////////////
// Estimator state consistency with its VMModel //
//////////////////////////////////////////////////

TEST_F(VMModelEstimatorTests, estimator_state_consistency_after_serialization_deserialization)
{
  fetch::serializers::MsgPackSerializer serializer;

  std::vector<SizeType> sizes          = {10, 10, 10};
  std::vector<bool>     activations    = {true, true};
  auto                  model_original = VmSequentialModel(vm, sizes, activations, "mse", "adam");
  auto                  model_from_serializer = VmSequentialModel(vm);

  model_original->SerializeTo(serializer);
  serializer.seek(0);
  model_from_serializer->DeserializeFrom(serializer);

  ChargeAmount charge_original        = 0;
  ChargeAmount charge_from_serializer = 0;

  //
  SizeType input_size  = 10;
  SizeType output_size = 100;
  auto     layer_type  = VmString(vm, "dense");
  charge_original      = LayerAddDenseCharge(model_original, layer_type, input_size, output_size);
  charge_from_serializer =
      LayerAddDenseCharge(model_from_serializer, layer_type, input_size, output_size);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  auto activation_type   = VmString(vm, "relu");
  charge_original        = LayerAddDenseActivationCharge(model_original, layer_type, input_size,
                                                  output_size, activation_type);
  charge_from_serializer = LayerAddDenseActivationCharge(model_from_serializer, layer_type,
                                                         input_size, output_size, activation_type);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  auto loss              = VmString(vm, "mse");
  auto optimiser         = VmString(vm, "adam");
  charge_original        = CompileSequentialCharge(model_original, loss, optimiser);
  charge_from_serializer = CompileSequentialCharge(model_from_serializer, loss, optimiser);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  SizeType              batch_size = 64;
  SizeType              datapoints = 128;
  std::vector<SizeType> data_shape{sizes[0], datapoints};
  std::vector<SizeType> label_shape{sizes[sizes.size() - 1], datapoints};
  auto                  data  = VmTensor(vm, data_shape);
  auto                  label = VmTensor(vm, label_shape);
  charge_original             = FitCharge(model_original, data, label, batch_size);
  charge_from_serializer      = FitCharge(model_from_serializer, data, label, batch_size);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  charge_original        = PredictCharge(model_original, data);
  charge_from_serializer = PredictCharge(model_from_serializer, data);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  charge_original        = SerializeToStringCharge(model_original);
  charge_from_serializer = SerializeToStringCharge(model_from_serializer);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  auto original_serialized        = model_original->SerializeToString();
  auto from_serializer_serialized = model_from_serializer->SerializeToString();
  charge_original = DeserializeFromStringCharge(model_original, original_serialized);
  charge_from_serializer =
      DeserializeFromStringCharge(model_from_serializer, from_serializer_serialized);
  EXPECT_EQ(charge_original, charge_from_serializer);
}

TEST_F(VMModelEstimatorTests,
       estimator_state_consistency_after_serialization_deserialization_from_string)
{
  std::vector<SizeType> sizes          = {10, 10, 10};
  std::vector<bool>     activations    = {true, true};
  auto                  model_original = VmSequentialModel(vm, sizes, activations, "mse", "adam");
  auto                  model_from_serializer = VmSequentialModel(vm);

  auto serialized_model = model_original->SerializeToString();
  model_from_serializer->DeserializeFromString(serialized_model);
  ChargeAmount charge_original        = 0;
  ChargeAmount charge_from_serializer = 0;

  //
  SizeType input_size  = 10;
  SizeType output_size = 100;
  auto     layer_type  = VmString(vm, "dense");
  charge_original      = LayerAddDenseCharge(model_original, layer_type, input_size, output_size);
  charge_from_serializer =
      LayerAddDenseCharge(model_from_serializer, layer_type, input_size, output_size);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  auto activation_type   = VmString(vm, "relu");
  charge_original        = LayerAddDenseActivationCharge(model_original, layer_type, input_size,
                                                  output_size, activation_type);
  charge_from_serializer = LayerAddDenseActivationCharge(model_from_serializer, layer_type,
                                                         input_size, output_size, activation_type);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  auto loss              = VmString(vm, "mse");
  auto optimiser         = VmString(vm, "adam");
  charge_original        = CompileSequentialCharge(model_original, loss, optimiser);
  charge_from_serializer = CompileSequentialCharge(model_from_serializer, loss, optimiser);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  SizeType              batch_size = 64;
  SizeType              datapoints = 128;
  std::vector<SizeType> data_shape{sizes[0], datapoints};
  std::vector<SizeType> label_shape{sizes[sizes.size() - 1], datapoints};
  auto                  data  = VmTensor(vm, data_shape);
  auto                  label = VmTensor(vm, label_shape);
  charge_original             = FitCharge(model_original, data, label, batch_size);
  charge_from_serializer      = FitCharge(model_from_serializer, data, label, batch_size);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  charge_original        = PredictCharge(model_original, data);
  charge_from_serializer = PredictCharge(model_from_serializer, data);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  charge_original        = SerializeToStringCharge(model_original);
  charge_from_serializer = SerializeToStringCharge(model_from_serializer);
  EXPECT_EQ(charge_original, charge_from_serializer);

  //
  auto original_serialized        = model_original->SerializeToString();
  auto from_serializer_serialized = model_from_serializer->SerializeToString();
  charge_original = DeserializeFromStringCharge(model_original, original_serialized);
  charge_from_serializer =
      DeserializeFromStringCharge(model_from_serializer, from_serializer_serialized);
  EXPECT_EQ(charge_original, charge_from_serializer);
}

TEST_F(VMModelEstimatorTests, charge_forward_one_dense)
{
  using namespace fetch::ml::charge_estimation::ops;
  auto              loss        = VmString(vm, "mse");
  auto              optimiser   = VmString(vm, "adam");
  std::vector<bool> activations = {true, true};

  SizeType const inputs     = 10;
  SizeType const outputs    = 10;
  SizeType const batch_size = 20;

  VmModelPtr  model = VmSequentialModel(vm);
  VmStringPtr dense{new fetch::vm::String(vm.get(), "dense")};

  model->LayerAddDense(dense, inputs, outputs);
  model->CompileSequential(loss, optimiser);

  auto data = VmTensor(vm, {inputs, batch_size});
  data->FillRandom();

  const ChargeAmount cost = model->EstimatePredict(data);

  ChargeAmount expected_cost = 0;
  // For a Dense layer with n inputs and m outputs and empty activation there is expected
  // n placeholder readings
  expected_cost += inputs * PLACEHOLDER_READING_PER_ELEMENT * batch_size;
  // n*m Weights reading (100 weights total)
  expected_cost += (inputs * outputs) * WEIGHTS_READING_PER_ELEMENT * batch_size;
  // There is a second placeholder - one is in the graph and the other in the FC layer
  expected_cost += inputs * PLACEHOLDER_READING_PER_ELEMENT * batch_size;
  // n flattening operations (because Dense is not time-distributed in this test)
  expected_cost += inputs * FLATTEN_PER_ELEMENT * batch_size;
  // n*m*1 matmul operations
  expected_cost += (inputs * outputs) * MULTIPLICATION_PER_ELEMENT * batch_size;
  // m bias weights reading
  expected_cost += outputs * WEIGHTS_READING_PER_ELEMENT * batch_size;

  fetch::ml::OperationsCount add_charge{1};

  // Addition cost
  // Type of tensor is not important for SizeFromShape
  fetch::math::SizeType num_elements =
      fetch::math::Tensor<float>::SizeFromShape({outputs, batch_size});
  add_charge += num_elements;

  // Iteration over 3 tensors (input1, input2, ret)
  // Type of tensor is not important for ChargeIterate
  fetch::ml::OperationsCount iteration_ops =
      fetch::math::Tensor<float>::ChargeIterate({outputs, batch_size});
  add_charge += iteration_ops * 3;

  // m adding (bias + matmul result)
  expected_cost += add_charge;

  ASSERT_EQ(cost, expected_cost);
}

}  // namespace
