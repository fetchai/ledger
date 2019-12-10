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

#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/ml/model/model_estimator.hpp"

#include <stdexcept>

using namespace fetch::vm;

namespace fetch {

namespace vm_modules {
namespace ml {
namespace model {

static constexpr char const *LOGGING_NAME = "VMModelEstimator";

using SizeType = fetch::math::SizeType;

ModelEstimator::ModelEstimator(VMObjectType &model)
  : model_{model}
{}

ModelEstimator &ModelEstimator::operator=(ModelEstimator const &other) noexcept
{
  CopyStateFrom(other);
  return *this;
}

ModelEstimator &ModelEstimator::operator=(ModelEstimator &&other) noexcept
{
  CopyStateFrom(other);
  return *this;
}

/**
 * Estimates and returns the cost of adding the relevant layer, but also updates internal state for
 * other calls (e.g. forward_pass_cost etc.).
 * Must be a sequential model
 * @param layer description of layer type
 * @param inputs number of inputs to layer
 * @param hidden_nodes number of outputs of layer
 * @return
 */
ChargeAmount ModelEstimator::LayerAddDense(Ptr<String> const &layer, SizeType const &inputs,
                                           SizeType const &hidden_nodes)
{
  SizeType padded_size{0};

  // must be a dense layer
  if(layer->string() != "dense")
  {
    return MaximumCharge("Only dense layers are supported");
  }

  state_.forward_pass_cost =
      state_.forward_pass_cost + static_cast<DataType>(inputs) * FORWARD_DENSE_INPUT_COEF();
  state_.forward_pass_cost =
      state_.forward_pass_cost + static_cast<DataType>(hidden_nodes) * FORWARD_DENSE_OUTPUT_COEF();
  state_.forward_pass_cost =
      state_.forward_pass_cost +
      static_cast<DataType>(inputs * hidden_nodes) * FORWARD_DENSE_QUAD_COEF();

  state_.backward_pass_cost =
      state_.backward_pass_cost + static_cast<DataType>(inputs) * BACKWARD_DENSE_INPUT_COEF();
  state_.backward_pass_cost = state_.backward_pass_cost +
                              static_cast<DataType>(hidden_nodes) * BACKWARD_DENSE_OUTPUT_COEF();
  state_.backward_pass_cost =
      state_.backward_pass_cost +
      static_cast<DataType>(inputs * hidden_nodes) * BACKWARD_DENSE_QUAD_COEF();

  state_.weights_size_sum += inputs * hidden_nodes + hidden_nodes;

  // DataType of Tensor is not important for caluclating padded size
  padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape({hidden_nodes, inputs});
  padded_size += fetch::math::Tensor<DataType>::PaddedSizeFromShape({hidden_nodes, 1});

  state_.weights_padded_size_sum += padded_size;
  state_.last_layer_size = hidden_nodes;
  state_.ops_count += 3;

  return static_cast<ChargeAmount>(
             ADD_DENSE_INPUT_COEF() * inputs + ADD_DENSE_OUTPUT_COEF() * hidden_nodes +
             ADD_DENSE_QUAD_COEF() * inputs * hidden_nodes + ADD_DENSE_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount ModelEstimator::LayerAddDenseActivation(Ptr<fetch::vm::String> const &layer,
                                                     SizeType const &              inputs,
                                                     SizeType const &              hidden_nodes,
                                                     Ptr<fetch::vm::String> const &activation)
{
  ChargeAmount estimate = LayerAddDense(layer, inputs, hidden_nodes);

  if(activation->string() != "relu")
  {
    return MaximumCharge("Only relu activations are supported");
  }

  state_.forward_pass_cost  = state_.forward_pass_cost + RELU_FORWARD_IMPACT() * hidden_nodes;
  state_.backward_pass_cost = state_.backward_pass_cost + RELU_BACKWARD_IMPACT() * hidden_nodes;
  state_.ops_count++;

  return estimate;
}

/**
 * Method for giving charge estimate for experimental layers
 * @param layer
 * @param inputs
 * @param hidden_nodes
 * @param activation
 * @return
 */
ChargeAmount ModelEstimator::LayerAddDenseActivationExperimental(
    Ptr<fetch::vm::String> const &layer, SizeType const &inputs, SizeType const &hidden_nodes,
    Ptr<fetch::vm::String> const &activation)
{

  FETCH_UNUSED(layer);
  FETCH_UNUSED(inputs);
  FETCH_UNUSED(hidden_nodes);
  FETCH_UNUSED(activation);

  return MaximumCharge("attempted to estimate unknown layer with unknown activation type");
}

ChargeAmount ModelEstimator::LayerAddConv(Ptr<String> const &layer, SizeType const &output_channels,
                                          SizeType const &input_channels,
                                          SizeType const &kernel_size, SizeType const &stride_size)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(output_channels);
  FETCH_UNUSED(input_channels);
  FETCH_UNUSED(kernel_size);
  FETCH_UNUSED(stride_size);
  return MaximumCharge("Not yet implemented");
}

ChargeAmount ModelEstimator::LayerAddConvActivation(
    Ptr<String> const &layer, SizeType const &output_channels, SizeType const &input_channels,
    SizeType const &kernel_size, SizeType const &stride_size, Ptr<String> const &activation)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(output_channels);
  FETCH_UNUSED(input_channels);
  FETCH_UNUSED(kernel_size);
  FETCH_UNUSED(stride_size);
  FETCH_UNUSED(activation);
  return MaximumCharge("Not yet implement");
}

ChargeAmount ModelEstimator::CompileSequential(Ptr<String> const &loss,
                                               Ptr<String> const &optimiser)
{
  DataType optimiser_construction_impact(0.0);

  bool success = false;

  if (!model_.model_->loss_set_)
  {
    if (loss->string() == "mse")
    {
      // loss_type = fetch::ml::ops::LossType::MEAN_SQUARE_ERROR;
      state_.forward_pass_cost =
          state_.forward_pass_cost + MSE_FORWARD_IMPACT() * state_.last_layer_size;
      state_.backward_pass_cost =
          state_.backward_pass_cost + MSE_BACKWARD_IMPACT() * state_.last_layer_size;
      state_.ops_count++;
      success = true;
    }
    else if (loss->string() == "cel")
    {
      // loss_type = fetch::ml::ops::LossType::CROSS_ENTROPY;
      state_.forward_pass_cost =
          state_.forward_pass_cost + CEL_FORWARD_IMPACT() * state_.last_layer_size;
      state_.backward_pass_cost =
          state_.backward_pass_cost + CEL_BACKWARD_IMPACT() * state_.last_layer_size;
      state_.ops_count++;
      success = true;
    }
    else if (loss->string() == "scel")
    {
      // loss_type = fetch::ml::ops::LossType::SOFTMAX_CROSS_ENTROPY;
      success = false;
    }
    else
    {
      success = false;
    }
  }

  if (!model_.model_->optimiser_set_)
  {
    if (optimiser->string() == "adagrad")
    {
      // optimiser_type = fetch::ml::OptimiserType::ADAGRAD;
      success = false;
    }
    else if (optimiser->string() == "adam")
    {
      // optimiser_type = fetch::ml::OptimiserType::ADAM;
      state_.optimiser_step_impact = ADAM_STEP_IMPACT_COEF();
      optimiser_construction_impact =
          ADAM_PADDED_WEIGHTS_SIZE_COEF() * state_.weights_padded_size_sum +
          ADAM_WEIGHTS_SIZE_COEF() * state_.weights_size_sum;
      success = true;
    }
    else if (optimiser->string() == "momentum")
    {
      // optimiser_type = fetch::ml::OptimiserType::MOMENTUM;
      success = false;
    }
    else if (optimiser->string() == "rmsprop")
    {
      //  optimiser_type = fetch::ml::OptimiserType::RMSPROP;
      success = false;
    }
    else if (optimiser->string() == "sgd")
    {
      // optimiser_type = fetch::ml::OptimiserType::SGD;
      state_.optimiser_step_impact = SGD_STEP_IMPACT_COEF();
      optimiser_construction_impact =
          SGD_PADDED_WEIGHTS_SIZE_COEF() * state_.weights_padded_size_sum +
          SGD_WEIGHTS_SIZE_COEF() * state_.weights_size_sum;
      success = true;
    }
    else
    {
      success = false;
    }
  }

  if (!success)
  {
    return MaximumCharge("Not yet implement");
  }

  return static_cast<ChargeAmount>(optimiser_construction_impact + COMPILE_CONST_COEF()) *
         COMPUTE_CHARGE_COST;
}

ChargeAmount ModelEstimator::CompileSimple(Ptr<String> const &         optimiser,
                                           Ptr<Array<SizeType>> const &in_layers)
{

  FETCH_UNUSED(optimiser);
  FETCH_UNUSED(in_layers);
  return MaximumCharge("Not yet implement");
}

ChargeAmount ModelEstimator::Fit(Ptr<math::VMTensor> const &data, Ptr<math::VMTensor> const &labels,
                                 SizeType const &batch_size)
{
  FETCH_UNUSED(labels);

  DataType estimate(0);
  SizeType subset_size       = data->GetTensor().shape().at(data->GetTensor().shape().size() - 1);
  SizeType number_of_batches = subset_size / batch_size;

  // Forward pass
  estimate = estimate + state_.forward_pass_cost * subset_size;
  estimate = estimate + PREDICT_BATCH_LAYER_COEF() * subset_size * state_.ops_count;
  estimate = estimate + PREDICT_CONST_COEF();

  // Backward pass
  estimate = estimate + state_.backward_pass_cost * subset_size;
  estimate = estimate + BACKWARD_BATCH_LAYER_COEF() * subset_size * state_.ops_count;
  estimate = estimate + BACKWARD_PER_BATCH_COEF() * number_of_batches;

  // Optimiser step
  estimate = estimate + static_cast<DataType>(number_of_batches) * state_.optimiser_step_impact *
                            state_.weights_size_sum;

  // Call overhead
  estimate = estimate + FIT_CONST_COEF();

  return static_cast<ChargeAmount>(estimate) * COMPUTE_CHARGE_COST;
}

ChargeAmount ModelEstimator::Evaluate()
{
  // Just return loss_, constant charge
  return static_cast<ChargeAmount>(CONSTANT_CHARGE);
}

ChargeAmount ModelEstimator::Predict(Ptr<math::VMTensor> const &data)
{
  DataType estimate{0};
  SizeType batch_size = data->GetTensor().shape().at(data->GetTensor().shape().size() - 1);

  estimate = estimate + state_.forward_pass_cost * batch_size;
  estimate = estimate + PREDICT_BATCH_LAYER_COEF() * batch_size * state_.ops_count;
  estimate = estimate + PREDICT_CONST_COEF();

  return static_cast<ChargeAmount>(estimate * COMPUTE_CHARGE_COST);
}

ChargeAmount ModelEstimator::SerializeToString()
{
  DataType estimate{0};
  estimate = estimate + SERIALISATION_PER_OP_COEF() * state_.ops_count;
  estimate = estimate + SERIALISATION_PADDED_WEIGHT_SUM_COEF() * state_.weights_padded_size_sum;
  estimate = estimate + SERIALISATION_WEIGHT_SUM_COEF() * state_.weights_size_sum;
  estimate = estimate + SERIALISATION_CONST_COEF();

  return static_cast<ChargeAmount>(estimate) * COMPUTE_CHARGE_COST;
}

ChargeAmount ModelEstimator::DeserializeFromString(Ptr<String> const &model_string)
{
  DataType estimate = DESERIALISATION_PER_CHAR_COEF() * model_string->string().size();
  return static_cast<ChargeAmount>(estimate + DESERIALISATION_CONST_COEF()) * COMPUTE_CHARGE_COST;
}

bool ModelEstimator::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  return state_.SerializeTo(buffer);
}

bool ModelEstimator::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  return state_.DeserializeFrom(buffer);
}

void ModelEstimator::CopyStateFrom(ModelEstimator const &src)
{
  state_ = src.state_;
}

ChargeAmount ModelEstimator::MaximumCharge(std::string const &log_msg)
{
  FETCH_LOG_ERROR(LOGGING_NAME, "operation charge is vm::MAXIMUM_CHARGE : " + log_msg);
  return vm::MAXIMUM_CHARGE;
}

bool ModelEstimator::State::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << forward_pass_cost;
  buffer << backward_pass_cost;
  buffer << weights_size_sum;
  buffer << optimiser_step_impact;
  buffer << last_layer_size;
  buffer << ops_count;

  return true;
}

bool ModelEstimator::State::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  buffer >> forward_pass_cost;
  buffer >> backward_pass_cost;
  buffer >> weights_size_sum;
  buffer >> optimiser_step_impact;
  buffer >> last_layer_size;
  buffer >> ops_count;

  return true;
}

fetch::math::SizeType ModelEstimator::GetPaddedSizesSum()
{
  return state_.weights_padded_size_sum;
}

fetch::math::SizeType ModelEstimator::GetSizesSum()
{
  return state_.weights_size_sum;
}

fetch::math::SizeType ModelEstimator::GetOpsCount()
{
  return state_.ops_count;
}

fetch::fixed_point::FixedPoint<32, 32> ModelEstimator::GetForwardCost()
{
  return state_.forward_pass_cost;
}

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
