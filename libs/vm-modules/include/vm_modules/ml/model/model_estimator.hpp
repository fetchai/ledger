#pragma once
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

#include "vm/common.hpp"
#include "vm_modules/math/tensor.hpp"

namespace fetch {

namespace vm {
struct String;
}

namespace vm_modules {
namespace math {
class VMTensor;
}
}  // namespace vm_modules

namespace vm_modules {
namespace ml {
namespace model {

class VMModel;

class ModelEstimator
{
public:
  using VMObjectType = VMModel;
  using ChargeAmount = fetch::vm::ChargeAmount;

  explicit ModelEstimator(VMObjectType &model);
  ~ModelEstimator() = default;

  ModelEstimator(ModelEstimator const &other)  = delete;
  ModelEstimator &operator                     =(ModelEstimator const &other);
  ModelEstimator(ModelEstimator const &&other) = delete;
  ModelEstimator &operator                     =(ModelEstimator const &&other);

  ChargeAmount LayerAddDense(fetch::vm::Ptr<fetch::vm::String> const &layer,
                             math::SizeType const &inputs, math::SizeType const &hidden_nodes);
  ChargeAmount LayerAddDenseActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                       math::SizeType const &                   inputs,
                                       math::SizeType const &                   hidden_nodes,
                                       fetch::vm::Ptr<fetch::vm::String> const &activation);

  ChargeAmount LayerAddConv(fetch::vm::Ptr<fetch::vm::String> const &layer,
                            math::SizeType const &                   output_channels,
                            math::SizeType const &input_channels, math::SizeType const &kernel_size,
                            math::SizeType const &stride_size);
  ChargeAmount LayerAddConvActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                      math::SizeType const &                   output_channels,
                                      math::SizeType const &                   input_channels,
                                      math::SizeType const &                   kernel_size,
                                      math::SizeType const &                   stride_size,
                                      fetch::vm::Ptr<fetch::vm::String> const &activation);

  ChargeAmount CompileSequential(fetch::vm::Ptr<fetch::vm::String> const &loss,
                                 fetch::vm::Ptr<fetch::vm::String> const &optimiser);

  ChargeAmount CompileSimple(fetch::vm::Ptr<fetch::vm::String> const &        optimiser,
                             fetch::vm::Ptr<vm::Array<math::SizeType>> const &in_layers);

  ChargeAmount Fit(vm::Ptr<vm_modules::math::VMTensor> const &data,
                   vm::Ptr<vm_modules::math::VMTensor> const &labels,
                   ::fetch::math::SizeType const &            batch_size);

  ChargeAmount Evaluate();

  ChargeAmount Predict(vm::Ptr<vm_modules::math::VMTensor> const &data);

  ChargeAmount SerializeToString();

  ChargeAmount DeserializeFromString(fetch::vm::Ptr<fetch::vm::String> const &model_string);

  bool SerializeTo(serializers::MsgPackSerializer &buffer);

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer);

private:
  VMObjectType &model_;

  // Layers
  ChargeAmount forward_pass_cost_{0};
  ChargeAmount backward_pass_cost_{0};

  // Loss function
  ChargeAmount loss_forward_pass_cost_{0};
  ChargeAmount loss_backward_pass_cost_{0};

  // Optimiser
  ChargeAmount weights_size_sum_{0};
  ChargeAmount optimiser_step_impact_{0};

  fetch::math::SizeType last_layer_size_{0};

  ChargeAmount const constant_charge{vm::CHARGE_UNIT};

  void copy_state_from(ModelEstimator const &);
};

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
