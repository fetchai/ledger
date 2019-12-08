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
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/ml/model/model_estimator.hpp"
#include "vm_test_toolkit.hpp"

namespace {

using SizeType         = fetch::math::SizeType;
using DataType         = fetch::vm_modules::math::DataType;
using VmPtr            = fetch::vm::Ptr<fetch::vm::String>;
using VmModel          = fetch::vm_modules::ml::model::VMModel;
using VmModelEstimator = fetch::vm_modules::ml::model::ModelEstimator;
using DataType         = fetch::vm_modules::ml::model::ModelEstimator::DataType;

class VMModelEstimatorTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

// sanity check that estimator behaves as intended
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

  VmPtr             vm_ptr_layer_type{new fetch::vm::String(&toolkit.vm(), layer_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(&toolkit.vm(), type_id, model_type);
  VmModelEstimator  model_estimator(model);

  for (SizeType inputs = min_input_size; inputs < max_input_size; inputs += input_step)
  {
    for (SizeType outputs = min_output_size; outputs < max_output_size; outputs += output_step)
    {
      DataType val = (VmModelEstimator::ADD_DENSE_INPUT_COEF() * inputs);
      val += VmModelEstimator::ADD_DENSE_OUTPUT_COEF() * outputs;
      val += VmModelEstimator::ADD_DENSE_QUAD_COEF() * inputs * outputs;
      val += VmModelEstimator::ADD_DENSE_CONST_COEF();

      EXPECT_TRUE(model_estimator.LayerAddDense(vm_ptr_layer_type, inputs, outputs) ==
                  static_cast<ChargeAmount>(val));
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(VMModelEstimatorTests, add_dense_layer_activation_test)
{
  std::string model_type      = "sequential";
  std::string layer_type      = "dense";
  std::string activation_type = "relu";

  SizeType min_input_size  = 0;
  SizeType max_input_size  = 1000;
  SizeType input_step      = 10;
  SizeType min_output_size = 0;
  SizeType max_output_size = 1000;
  SizeType output_step     = 10;

  VmPtr             vm_ptr_layer_type{new fetch::vm::String(&toolkit.vm(), layer_type)};
  VmPtr             vm_ptr_activation_type{new fetch::vm::String(&toolkit.vm(), activation_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(&toolkit.vm(), type_id, model_type);
  VmModelEstimator  model_estimator(model);

  for (SizeType inputs = min_input_size; inputs < max_input_size; inputs += input_step)
  {
    for (SizeType outputs = min_output_size; outputs < max_output_size; outputs += output_step)
    {
      DataType val = (VmModelEstimator::ADD_DENSE_INPUT_COEF() * inputs);
      val += VmModelEstimator::ADD_DENSE_OUTPUT_COEF() * outputs;
      val += VmModelEstimator::ADD_DENSE_QUAD_COEF() * inputs * outputs;
      val += VmModelEstimator::ADD_DENSE_CONST_COEF();

      EXPECT_TRUE(model_estimator.LayerAddDenseActivation(vm_ptr_layer_type, inputs, outputs,
                                                          vm_ptr_activation_type) ==
                  static_cast<ChargeAmount>(val));
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(VMModelEstimatorTests, add_conv_layer_test)
{
  std::string model_type = "sequential";
  std::string layer_type = "convolution1D";

  SizeType min_input_size = 0;
  SizeType max_input_size = 1000;
  SizeType input_step     = 10;

  SizeType min_output_size = 0;
  SizeType max_output_size = 1000;
  SizeType output_step     = 10;

  SizeType min_kernel_size = 0;
  SizeType max_kernel_size = 100;
  SizeType kernel_step     = 10;

  SizeType min_stride_size = 0;
  SizeType max_stride_size = 100;
  SizeType stride_step     = 10;

  VmPtr             vm_ptr_layer_type{new fetch::vm::String(&toolkit.vm(), layer_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(&toolkit.vm(), type_id, model_type);
  VmModelEstimator  model_estimator(model);

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

          EXPECT_TRUE(model_estimator.LayerAddConv(vm_ptr_layer_type, output_channels,
                                                   input_channels, kernel_size, stride_size) ==
                      static_cast<ChargeAmount>(fetch::vm::MAXIMUM_CHARGE));
        }
      }
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(VMModelEstimatorTests, add_conv_layer_activation_test)
{
  std::string model_type      = "sequential";
  std::string layer_type      = "convolution1D";
  std::string activation_type = "relu";

  SizeType min_input_size = 0;
  SizeType max_input_size = 1000;
  SizeType input_step     = 10;

  SizeType min_output_size = 0;
  SizeType max_output_size = 1000;
  SizeType output_step     = 10;

  SizeType min_kernel_size = 0;
  SizeType max_kernel_size = 100;
  SizeType kernel_step     = 10;

  SizeType min_stride_size = 0;
  SizeType max_stride_size = 100;
  SizeType stride_step     = 10;

  VmPtr             vm_ptr_layer_type{new fetch::vm::String(&toolkit.vm(), layer_type)};
  VmPtr             vm_ptr_activation_type{new fetch::vm::String(&toolkit.vm(), activation_type)};
  fetch::vm::TypeId type_id = 0;
  VmModel           model(&toolkit.vm(), type_id, model_type);
  VmModelEstimator  model_estimator(model);

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

          EXPECT_TRUE(model_estimator.LayerAddConvActivation(vm_ptr_layer_type, output_channels,
                                                             input_channels, kernel_size,
                                                             stride_size, vm_ptr_activation_type) ==
                      static_cast<ChargeAmount>(fetch::vm::MAXIMUM_CHARGE));
        }
      }
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(VMModelEstimatorTests, compile_sequential_test)
{
  std::string model_type = "sequential";
  std::string layer_type = "dense";
  std::string loss_type  = "mse";
  std::string opt_type   = "adam";

  SizeType min_input_size  = 0;
  SizeType max_input_size  = 1000;
  SizeType input_step      = 10;
  SizeType min_output_size = 0;
  SizeType max_output_size = 1000;
  SizeType output_step     = 10;

  fetch::vm::TypeId type_id = 0;

  VmPtr vm_ptr_layer_type{new fetch::vm::String(&toolkit.vm(), layer_type)};
  VmPtr vm_ptr_loss_type{new fetch::vm::String(&toolkit.vm(), loss_type)};
  VmPtr vm_ptr_opt_type{new fetch::vm::String(&toolkit.vm(), opt_type)};

  for (SizeType inputs = min_input_size; inputs < max_input_size; inputs += input_step)
  {
    for (SizeType outputs = min_output_size; outputs < max_output_size; outputs += output_step)
    {
      VmModel          model(&toolkit.vm(), type_id, model_type);
      VmModelEstimator model_estimator(model);

      // add some layers
      model_estimator.LayerAddDense(vm_ptr_layer_type, inputs, outputs);
      SizeType weights_padded_size =
          fetch::math::Tensor<DataType>::PaddedSizeFromShape({outputs, inputs});
      weights_padded_size += fetch::math::Tensor<DataType>::PaddedSizeFromShape({outputs, 1});
      SizeType weights_size_sum = inputs * outputs + outputs;

      model_estimator.LayerAddDense(vm_ptr_layer_type, inputs, outputs);
      weights_padded_size += fetch::math::Tensor<DataType>::PaddedSizeFromShape({outputs, inputs});
      weights_padded_size += fetch::math::Tensor<DataType>::PaddedSizeFromShape({outputs, 1});
      weights_size_sum += inputs * outputs + outputs;

      model_estimator.LayerAddDense(vm_ptr_layer_type, inputs, outputs);
      weights_padded_size += fetch::math::Tensor<DataType>::PaddedSizeFromShape({outputs, inputs});
      weights_padded_size += fetch::math::Tensor<DataType>::PaddedSizeFromShape({outputs, 1});
      weights_size_sum += inputs * outputs + outputs;

      DataType val = VmModelEstimator::ADAM_PADDED_WEIGHTS_SIZE_COEF() * weights_padded_size;
      val += VmModelEstimator::ADAM_WEIGHTS_SIZE_COEF() * weights_size_sum;
      val += VmModelEstimator::COMPILE_CONST_COEF();

      EXPECT_TRUE(model_estimator.CompileSequential(vm_ptr_loss_type, vm_ptr_opt_type) ==
                  static_cast<ChargeAmount>(val));
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(VMModelEstimatorTests, compile_simple_test)
{
  std::string model_type = "regressor";
  std::string opt_type   = "adam";

  SizeType min_layer_size = 0;
  SizeType max_layer_size = 5;
  SizeType layer_step     = 1;

  fetch::vm::TypeId type_id = 0;

  VmPtr vm_ptr_opt_type{new fetch::vm::String(&toolkit.vm(), opt_type)};

  for (SizeType layers = min_layer_size; layers < max_layer_size; layers += layer_step)
  {

    fetch::vm::Ptr<fetch::vm::Array<SizeType>> vm_ptr_layers{};
    VmModel                                    model(&toolkit.vm(), type_id, model_type);
    VmModelEstimator                           model_estimator(model);

    EXPECT_TRUE(model_estimator.CompileSimple(vm_ptr_opt_type, vm_ptr_layers) ==
                static_cast<ChargeAmount>(fetch::vm::MAXIMUM_CHARGE));
  }
}

// sanity check that estimator behaves as intended
TEST_F(VMModelEstimatorTests, estimator_fit_and_predict_test)
{
  std::string model_type      = "sequential";
  std::string layer_type      = "dense";
  std::string loss_type       = "mse";
  std::string opt_type        = "adam";
  std::string activation_type = "relu";

  SizeType min_data_size_1  = 10;
  SizeType max_data_size_1  = 100;
  SizeType data_size_1_step = 19;

  SizeType min_data_points  = 10;
  SizeType max_data_points  = 100;
  SizeType data_points_step = 13;

  SizeType min_label_size_1  = 1;
  SizeType max_label_size_1  = 100;
  SizeType label_size_1_step = 17;

  SizeType min_batch_size  = 1;
  SizeType batch_size_step = 23;

  fetch::vm::TypeId type_id = 0;
  VmPtr             vm_ptr_layer_type{new fetch::vm::String(&toolkit.vm(), layer_type)};
  VmPtr             vm_ptr_loss_type{new fetch::vm::String(&toolkit.vm(), loss_type)};
  VmPtr             vm_ptr_opt_type{new fetch::vm::String(&toolkit.vm(), opt_type)};
  VmPtr             vm_ptr_activation_type{new fetch::vm::String(&toolkit.vm(), activation_type)};

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

          SizeType weights_size_sum = data_size_1 * label_size_1 + label_size_1;

          std::vector<uint64_t>                             data_shape{{data_size_1, n_data}};
          std::vector<uint64_t>                             label_shape{{label_size_1, n_data}};
          fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> vm_ptr_tensor_data{
              new fetch::vm_modules::math::VMTensor(&toolkit.vm(), type_id, data_shape)};
          fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> vm_ptr_tensor_labels{
              new fetch::vm_modules::math::VMTensor(&toolkit.vm(), type_id, label_shape)};

          VmModel          model(&toolkit.vm(), type_id, model_type);
          VmModelEstimator model_estimator(model);

          model_estimator.LayerAddDenseActivation(vm_ptr_layer_type, data_size_1, label_size_1,
                                                  vm_ptr_activation_type);
          model.LayerAddDenseActivation(vm_ptr_layer_type, data_size_1, label_size_1,
                                        vm_ptr_activation_type);

          SizeType ops_count = 0;
          ops_count += 3;  // for dense layer
          ops_count += 1;  // for relu

          DataType forward_pass_cost =
              DataType(data_size_1) * VmModelEstimator::FORWARD_DENSE_INPUT_COEF();
          forward_pass_cost +=
              DataType(label_size_1) * VmModelEstimator::FORWARD_DENSE_OUTPUT_COEF();
          forward_pass_cost +=
              DataType(data_size_1 * label_size_1) * VmModelEstimator::FORWARD_DENSE_QUAD_COEF();
          forward_pass_cost += DataType(label_size_1) * VmModelEstimator::RELU_FORWARD_IMPACT();

          DataType backward_pass_cost =
              DataType(data_size_1) * VmModelEstimator::BACKWARD_DENSE_INPUT_COEF();
          backward_pass_cost +=
              DataType(label_size_1) * VmModelEstimator::BACKWARD_DENSE_OUTPUT_COEF();
          backward_pass_cost +=
              DataType(data_size_1 * label_size_1) * VmModelEstimator::BACKWARD_DENSE_QUAD_COEF();
          backward_pass_cost += DataType(label_size_1) * VmModelEstimator::RELU_BACKWARD_IMPACT();

          model_estimator.CompileSequential(vm_ptr_loss_type, vm_ptr_opt_type);
          model.CompileSequential(vm_ptr_loss_type, vm_ptr_opt_type);

          ops_count += 1;  // for loss

          forward_pass_cost += DataType(label_size_1) * VmModelEstimator::MSE_FORWARD_IMPACT();
          backward_pass_cost += DataType(label_size_1) * VmModelEstimator::MSE_BACKWARD_IMPACT();

          SizeType val = vm_ptr_tensor_data->GetTensor().size();
          val += vm_ptr_tensor_labels->GetTensor().size();
          val += VmModelEstimator::FIT_CONST_OVERHEAD;
          val += VmModelEstimator::FIT_PER_BATCH_OVERHEAD * (n_data / batch_size);
          val += vm_ptr_tensor_data->GetTensor().size();
          val += vm_ptr_tensor_labels->GetTensor().size();
          val += (n_data / batch_size);
          val += n_data * static_cast<SizeType>(forward_pass_cost + backward_pass_cost);
          val += static_cast<SizeType>(
              static_cast<DataType>(n_data / batch_size) *
              static_cast<DataType>(VmModelEstimator::ADAM_STEP_IMPACT_COEF() *
                                        DataType(weights_size_sum) +
                                    DataType(weights_size_sum)));

          EXPECT_TRUE(model_estimator.Fit(vm_ptr_tensor_data, vm_ptr_tensor_labels, batch_size) ==
                      val);

          EXPECT_TRUE(model_estimator.Evaluate() == VmModelEstimator::constant_charge);

          auto predict_val =
              static_cast<SizeType>(static_cast<DataType>(n_data) * forward_pass_cost);
          predict_val += static_cast<SizeType>(static_cast<DataType>(n_data * ops_count) *
                                               VmModelEstimator::PREDICT_BATCH_LAYER_COEF());
          predict_val += static_cast<SizeType>(VmModelEstimator::PREDICT_CONST_COEF());

          EXPECT_TRUE(model_estimator.Predict(vm_ptr_tensor_data) ==
                      static_cast<SizeType>(predict_val));
        }
      }
    }
  }
}

}  // namespace
