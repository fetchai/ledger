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

ChargeAmount ModelEstimator::LayerAddDense(Ptr<String> const &layer, math::SizeType const &inputs,
                                           math::SizeType const &hidden_nodes)
{
  FETCH_UNUSED(model_);
  FETCH_UNUSED(layer);
  FETCH_UNUSED(inputs);
  FETCH_UNUSED(hidden_nodes);
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::LayerAddDenseActivation(Ptr<fetch::vm::String> const &layer,
                                                     math::SizeType const &        inputs,
                                                     math::SizeType const &        hidden_nodes,
                                                     Ptr<fetch::vm::String> const &activation)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(inputs);
  FETCH_UNUSED(hidden_nodes);
  FETCH_UNUSED(activation);
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::LayerAddConv(Ptr<String> const &   layer,
                                          math::SizeType const &output_channels,
                                          math::SizeType const &input_channels,
                                          math::SizeType const &kernel_size,
                                          math::SizeType const &stride_size)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(output_channels);
  FETCH_UNUSED(input_channels);
  FETCH_UNUSED(kernel_size);
  FETCH_UNUSED(stride_size);
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::LayerAddConvActivation(Ptr<String> const &   layer,
                                                    math::SizeType const &output_channels,
                                                    math::SizeType const &input_channels,
                                                    math::SizeType const &kernel_size,
                                                    math::SizeType const &stride_size,
                                                    Ptr<String> const &   activation)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(output_channels);
  FETCH_UNUSED(input_channels);
  FETCH_UNUSED(kernel_size);
  FETCH_UNUSED(stride_size);
  FETCH_UNUSED(activation);
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::CompileSequential(Ptr<String> const &loss,
                                               Ptr<String> const &optimiser)
{
  FETCH_UNUSED(loss);
  FETCH_UNUSED(optimiser);
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::CompileSimple(Ptr<String> const &               optimiser,
                                           Ptr<Array<math::SizeType>> const &in_layers)
{
  FETCH_UNUSED(optimiser);
  FETCH_UNUSED(in_layers);
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::Fit(Ptr<math::VMTensor> const &data, Ptr<math::VMTensor> const &labels,
                                 ::fetch::math::SizeType const &batch_size)
{
  FETCH_UNUSED(data);
  FETCH_UNUSED(labels);
  FETCH_UNUSED(batch_size);
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::Evaluate()
{
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::Predict(Ptr<math::VMTensor> const &data)
{
  FETCH_UNUSED(data);
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::SerializeToString()
{
  return static_cast<ChargeAmount>(constant_charge);
}

ChargeAmount ModelEstimator::DeserializeFromString(Ptr<String> const &model_string)
{
  FETCH_UNUSED(model_string);
  return static_cast<ChargeAmount>(this->constant_charge);
}

bool ModelEstimator::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  FETCH_UNUSED(buffer);
  return false;
}

bool ModelEstimator::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  FETCH_UNUSED(buffer);
  return false;
}

void ModelEstimator::copy_state_from(ModelEstimator const & /*src*/)
{
  // throw std::runtime_error{"implement ModelEstimator::copy_state_from"};
}

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
