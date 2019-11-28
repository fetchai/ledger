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
    throw std::runtime_error("invalid params specified for " + layer->string() + " layer");
  }

  if (model_.model_category_ == ModelCategory::SEQUENTIAL)
  {
    forward_pass_cost_ += inputs * hidden_nodes + inputs + hidden_nodes;
    backward_pass_cost_ += 2 * inputs * hidden_nodes + inputs + 2 * hidden_nodes;
    weights_size_sum_ += inputs * hidden_nodes + hidden_nodes;
    last_layer_size_ = hidden_nodes;
    ops_count_ += 3;
  }
  else
  {
    throw std::runtime_error("no add method for non-sequential methods");
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
    forward_pass_cost_ += hidden_nodes;
    backward_pass_cost_ += hidden_nodes;
    ops_count_ += 1;
  }
  else
  {
    throw std::runtime_error("attempted to estimate unknown layer with unknown activation type");
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
  throw std::runtime_error("Not yet implement");
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
  throw std::runtime_error("Not yet implement");
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
      forward_pass_cost_ += MSE_FORWARD_IMPACT * last_layer_size_;
      backward_pass_cost_ += MSE_BACKWARD_IMPACT * last_layer_size_;
      ops_count_ += 1;
    }
    else if (loss->string() == "cel")
    {
      // loss_type = fetch::ml::ops::LossType::CROSS_ENTROPY;
      throw std::runtime_error("Not yet implement");
    }
    else if (loss->string() == "scel")
    {
      // loss_type = fetch::ml::ops::LossType::SOFTMAX_CROSS_ENTROPY;
      throw std::runtime_error("Not yet implement");
    }
    else
    {
      throw std::runtime_error("invalid loss function");
    }
  }

  if (!model_.GetModel()->optimiser_set_)
  {
    if (optimiser->string() == "adagrad")
    {
      // optimiser_type = fetch::ml::OptimiserType::ADAGRAD;
      throw std::runtime_error("Not yet implement");
    }
    else if (optimiser->string() == "adam")
    {
      // optimiser_type = fetch::ml::OptimiserType::ADAM;
      optimiser_step_impact_        = ADAM_STEP_IMPACT;
      optimiser_construction_impact = ADAM_CONSTRUCTION_IMPACT;
      ops_count_++;
    }
    else if (optimiser->string() == "momentum")
    {
      // optimiser_type = fetch::ml::OptimiserType::MOMENTUM;
      throw std::runtime_error("Not yet implement");
    }
    else if (optimiser->string() == "rmsprop")
    {
      //  optimiser_type = fetch::ml::OptimiserType::RMSPROP;
      throw std::runtime_error("Not yet implement");
    }
    else if (optimiser->string() == "sgd")
    {
      // optimiser_type = fetch::ml::OptimiserType::SGD;
      throw std::runtime_error("Not yet implement");
    }
    else
    {
      throw std::runtime_error("invalid optimiser");
    }
  }

  return static_cast<ChargeAmount>(optimiser_construction_impact * CHARGE_UNIT);
}

ChargeAmount ModelEstimator::CompileSimple(Ptr<String> const &         optimiser,
                                           Ptr<Array<SizeType>> const &in_layers)
{

  FETCH_UNUSED(optimiser);
  FETCH_UNUSED(in_layers);
  throw std::runtime_error("Not yet implement");
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
  estimate += subset_size * (forward_pass_cost_ + backward_pass_cost_);
  // Optimiser step and clearing gradients
  estimate +=
      (subset_size / batch_size) * (weights_size_sum_ * optimiser_step_impact_ + weights_size_sum_);

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
  SizeType estimate   = (forward_pass_cost_)*batch_size;

  return static_cast<ChargeAmount>(estimate) * CHARGE_UNIT;
}

ChargeAmount ModelEstimator::SerializeToString()
{
  SizeType estimate =
      ops_count_ * SERIALISATION_OVERHEAD + weights_size_sum_ * WEIGHT_SERIALISATION_OVERHEAD;
  return static_cast<ChargeAmount>(estimate) * CHARGE_UNIT;
}

ChargeAmount ModelEstimator::DeserializeFromString(Ptr<String> const &model_string)
{
  SizeType estimate = model_string->string().size() * DESERIALISATION_OVERHEAD;
  return static_cast<ChargeAmount>(estimate) * CHARGE_UNIT;
}

bool ModelEstimator::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << forward_pass_cost_;
  buffer << backward_pass_cost_;
  buffer << weights_size_sum_;
  buffer << optimiser_step_impact_;
  buffer << optimiser_step_impact_;
  buffer << last_layer_size_;
  buffer << ops_count_;

  return false;
}

bool ModelEstimator::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  buffer >> forward_pass_cost_;
  buffer >> backward_pass_cost_;
  buffer >> weights_size_sum_;
  buffer >> optimiser_step_impact_;
  buffer >> optimiser_step_impact_;
  buffer >> last_layer_size_;
  buffer >> ops_count_;

  return false;
}

void ModelEstimator::copy_state_from(ModelEstimator const &src)
{
  FETCH_UNUSED(src);
  throw std::runtime_error("Not yet implemented");
}

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
