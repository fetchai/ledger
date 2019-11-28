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

#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/ml/model/model_estimator.hpp"

#include <stdexcept>

using namespace fetch::vm;

namespace fetch {

namespace vm_modules {
namespace ml {
namespace model {

static constexpr char const *LOGGING_NAME = "VMModelEstimator";

ModelEstimator::ModelEstimator(VMObjectType &model)
  : model_{model}
{}

ModelEstimator &ModelEstimator::operator=(ModelEstimator const &other)
{
  copy_state_from(other);
  return *this;
}

ModelEstimator &ModelEstimator::operator=(ModelEstimator const &&other)
{
  copy_state_from(other);
  return *this;
}

ChargeAmount ModelEstimator::LayerAddDense(Ptr<String> const &layer, SizeType const &inputs,
                                           SizeType const &hidden_nodes)
{
  // guarantee it's a dense layer
  if (!(layer->string() == "dense"))
  {
    return infinite_charge("invalid params specified for " + layer->string() + " layer");
  }

  if (model_.model_category_ == ModelCategory::SEQUENTIAL)
  {
    state_.forward_pass_cost += inputs * hidden_nodes + inputs + hidden_nodes;
    state_.backward_pass_cost += 2 * inputs * hidden_nodes + inputs + 2 * hidden_nodes;
    state_.weights_size_sum += inputs * hidden_nodes + hidden_nodes;
    state_.last_layer_size = hidden_nodes;
    state_.ops_count += 3;
  }
  else
  {
    return infinite_charge("no add method for non-sequential methods");
  }

  return static_cast<ChargeAmount>((inputs * hidden_nodes + hidden_nodes) * CHARGE_UNIT);
}

ChargeAmount ModelEstimator::LayerAddDenseActivation(Ptr<fetch::vm::String> const &layer,
                                                     SizeType const &              inputs,
                                                     SizeType const &              hidden_nodes,
                                                     Ptr<fetch::vm::String> const &activation)
{
  ChargeAmount estimate = LayerAddDense(layer, inputs, hidden_nodes);

  if (activation->string() == "relu")
  {
    state_.forward_pass_cost += hidden_nodes;
    state_.backward_pass_cost += hidden_nodes;
    state_.ops_count += 1;
  }
  else
  {
    return infinite_charge("attempted to estimate unknown layer with unknown activation type");
  }

  return estimate;
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
  return infinite_charge("Not yet implement");
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
  return infinite_charge("Not yet implement");
}

ChargeAmount ModelEstimator::CompileSequential(Ptr<String> const &loss,
                                               Ptr<String> const &optimiser)
{
  SizeType optimiser_construction_impact = 0;

  if (!model_.GetModel()->loss_set_)
  {
    if (loss->string() == "mse")
    {
      // loss_type = fetch::ml::ops::LossType::MEAN_SQUARE_ERROR;
      state_.forward_pass_cost += MSE_FORWARD_IMPACT * state_.last_layer_size;
      state_.backward_pass_cost += MSE_BACKWARD_IMPACT * state_.last_layer_size;
      state_.ops_count += 1;
    }
    else if (loss->string() == "cel")
    {
      // loss_type = fetch::ml::ops::LossType::CROSS_ENTROPY;
      return infinite_charge("Not yet implement");
    }
    else if (loss->string() == "scel")
    {
      // loss_type = fetch::ml::ops::LossType::SOFTMAX_CROSS_ENTROPY;
      return infinite_charge("Not yet implement");
    }
    else
    {
      return infinite_charge("invalid loss function");
    }
  }

  if (!model_.GetModel()->optimiser_set_)
  {
    if (optimiser->string() == "adagrad")
    {
      // optimiser_type = fetch::ml::OptimiserType::ADAGRAD;
      return vm::CHARGE_INFINITY;
    }
    else if (optimiser->string() == "adam")
    {
      // optimiser_type = fetch::ml::OptimiserType::ADAM;
      state_.optimiser_step_impact  = ADAM_STEP_IMPACT;
      optimiser_construction_impact = ADAM_CONSTRUCTION_IMPACT;
      state_.ops_count++;
    }
    else if (optimiser->string() == "momentum")
    {
      // optimiser_type = fetch::ml::OptimiserType::MOMENTUM;
      return infinite_charge("Not yet implement");
      return vm::CHARGE_INFINITY;
    }
    else if (optimiser->string() == "rmsprop")
    {
      //  optimiser_type = fetch::ml::OptimiserType::RMSPROP;
      return infinite_charge("Not yet implement");
      return vm::CHARGE_INFINITY;
    }
    else if (optimiser->string() == "sgd")
    {
      // optimiser_type = fetch::ml::OptimiserType::SGD;
      return infinite_charge("Not yet implement");
      return vm::CHARGE_INFINITY;
    }
    else
    {
      return infinite_charge("invalid optimiser");
      return vm::CHARGE_INFINITY;
    }
  }

  return static_cast<ChargeAmount>(optimiser_construction_impact * CHARGE_UNIT);
}

ChargeAmount ModelEstimator::CompileSimple(Ptr<String> const &         optimiser,
                                           Ptr<Array<SizeType>> const &in_layers)
{

  FETCH_UNUSED(optimiser);
  FETCH_UNUSED(in_layers);
  return infinite_charge("Not yet implement");
}

ChargeAmount ModelEstimator::Fit(Ptr<math::VMTensor> const &data, Ptr<math::VMTensor> const &labels,
                                 SizeType const &batch_size)
{
  SizeType estimate;
  SizeType subset_size = data->GetTensor().shape().at(data->GetTensor().shape().size() - 1);
  SizeType data_size   = data->GetTensor().size();
  SizeType labels_size = labels->GetTensor().size();

  // Assign input data to dataloader
  estimate = data_size;
  // Assign label data to dataloader
  estimate += labels_size;
  // SetRandomMode, UpdateConfig, etc.
  estimate += FIT_CONST_OVERHEAD;
  // PrepareBatch overhead
  estimate += FIT_PER_BATCH_OVERHEAD * subset_size / batch_size;
  // PrepareBatch-input
  estimate += data_size;
  // PrepareBatch-label
  estimate += labels_size;
  // SetInputReference, update stats
  estimate += subset_size / batch_size;
  // Forward and backward prob
  estimate += subset_size * (state_.forward_pass_cost + state_.backward_pass_cost);
  // Optimiser step and clearing gradients
  estimate += (subset_size / batch_size) *
              (state_.weights_size_sum * state_.optimiser_step_impact + state_.weights_size_sum);

  return static_cast<ChargeAmount>(estimate) * CHARGE_UNIT;
}

ChargeAmount ModelEstimator::Evaluate()
{
  // Just return loss_, constant charge
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::Predict(Ptr<math::VMTensor> const &data)
{
  SizeType batch_size = data->GetTensor().shape().at(data->GetTensor().shape().size() - 1);
  SizeType estimate   = (state_.forward_pass_cost) * batch_size;

  return static_cast<ChargeAmount>(estimate) * CHARGE_UNIT;
}

ChargeAmount ModelEstimator::SerializeToString()
{
  SizeType estimate = state_.ops_count * SERIALISATION_OVERHEAD +
                      state_.weights_size_sum * WEIGHT_SERIALISATION_OVERHEAD;
  return static_cast<ChargeAmount>(estimate) * CHARGE_UNIT;
}

ChargeAmount ModelEstimator::DeserializeFromString(Ptr<String> const &model_string)
{
  SizeType estimate = model_string->string().size() * DESERIALISATION_OVERHEAD;
  return static_cast<ChargeAmount>(estimate) * CHARGE_UNIT;
}

bool ModelEstimator::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  return state_.SerializeTo(buffer);
}

bool ModelEstimator::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  return state_.DeserializeFrom(buffer);
}

void ModelEstimator::copy_state_from(ModelEstimator const &src)
{
  state_ = src.state_;
}

ChargeAmount ModelEstimator::infinite_charge(std::string const &log_msg)
{
  FETCH_LOG_ERROR(LOGGING_NAME, "operation charge is vm::CHARGE_INIFITY : " + log_msg);
  return vm::CHARGE_INFINITY;
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

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
