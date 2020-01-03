#pragma once
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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/common.hpp"
#include "vm_modules/math/tensor/tensor.hpp"

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
  using SizeType     = fetch::math::SizeType;
  using DataType     = fetch::fixed_point::FixedPoint<32, 32>;

  explicit ModelEstimator(VMObjectType &model);
  ~ModelEstimator() = default;

  ModelEstimator(ModelEstimator const &other) = delete;
  ModelEstimator &operator                    =(ModelEstimator const &other) noexcept;
  ModelEstimator(ModelEstimator &&other)      = delete;
  ModelEstimator &operator                    =(ModelEstimator &&other) noexcept;

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
  ChargeAmount LayerAddDenseActivationExperimental(
      fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &inputs,
      math::SizeType const &hidden_nodes, fetch::vm::Ptr<fetch::vm::String> const &activation);

  ChargeAmount LayerAddFlatten(fetch::vm::Ptr<fetch::vm::String> const &layer);

  ChargeAmount LayerAddDropout(fetch::vm::Ptr<fetch::vm::String> const &layer,
                               math::DataType const &                   probability);

  ChargeAmount LayerAddActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                  fetch::vm::Ptr<fetch::vm::String> const &activation);
  ChargeAmount LayerAddReshape(fetch::vm::Ptr<fetch::vm::String> const &               layer,
                               fetch::vm::Ptr<fetch::vm::Array<math::SizeType>> const &shape);

  ChargeAmount CompileSequential(fetch::vm::Ptr<fetch::vm::String> const &loss,
                                 fetch::vm::Ptr<fetch::vm::String> const &optimiser);

  ChargeAmount CompileSequentialWithMetrics(
      vm::Ptr<vm::String> const &loss, vm::Ptr<vm::String> const &optimiser,
      vm::Ptr<vm::Array<vm::Ptr<fetch::vm::String>>> const &metrics);

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

  SizeType GetPaddedSizesSum();
  SizeType GetSizesSum();
  SizeType GetOpsCount();
  DataType GetForwardCost();

  // AddLayer
  static const fixed_point::fp64_t ADD_DENSE_PADDED_WEIGHTS_SIZE_COEF;
  static const fixed_point::fp64_t ADD_DENSE_WEIGHTS_SIZE_COEF;
  static const fixed_point::fp64_t ADD_DENSE_CONST_COEF;

  // Compile
  static const fixed_point::fp64_t ADAM_PADDED_WEIGHTS_SIZE_COEF;
  static const fixed_point::fp64_t ADAM_WEIGHTS_SIZE_COEF;
  static const fixed_point::fp64_t ADAM_STEP_IMPACT_COEF;

  static const fixed_point::fp64_t SGD_PADDED_WEIGHTS_SIZE_COEF;
  static const fixed_point::fp64_t SGD_WEIGHTS_SIZE_COEF;
  static const fixed_point::fp64_t SGD_STEP_IMPACT_COEF;
  static const fixed_point::fp64_t COMPILE_CONST_COEF;

  // Forward
  static const fixed_point::fp64_t FORWARD_DENSE_INPUT_COEF;

  static const fixed_point::fp64_t FORWARD_DENSE_OUTPUT_COEF;

  static const fixed_point::fp64_t FORWARD_DENSE_QUAD_COEF;

  static const fixed_point::fp64_t RELU_FORWARD_IMPACT;

  static const fixed_point::fp64_t MSE_FORWARD_IMPACT;

  static const fixed_point::fp64_t CEL_FORWARD_IMPACT;

  static const fixed_point::fp64_t SCEL_FORWARD_IMPACT;

  static const fixed_point::fp64_t CATEGORICAL_ACCURACY_FORWARD_IMPACT;

  // Backward
  static const fixed_point::fp64_t BACKWARD_DENSE_INPUT_COEF;

  static const fixed_point::fp64_t BACKWARD_DENSE_OUTPUT_COEF;

  static const fixed_point::fp64_t BACKWARD_DENSE_QUAD_COEF;

  static const fixed_point::fp64_t RELU_BACKWARD_IMPACT;

  static const fixed_point::fp64_t MSE_BACKWARD_IMPACT;

  static const fixed_point::fp64_t CEL_BACKWARD_IMPACT;

  static const fixed_point::fp64_t SCEL_BACKWARD_IMPACT;

  // Predict
  static const fixed_point::fp64_t PREDICT_BATCH_LAYER_COEF;

  static const fixed_point::fp64_t PREDICT_CONST_COEF;

  // Fit
  static const fixed_point::fp64_t BACKWARD_BATCH_LAYER_COEF;
  static const fixed_point::fp64_t BACKWARD_PER_BATCH_COEF;
  static const fixed_point::fp64_t FIT_CONST_COEF;

  // Deserialisation
  static const fixed_point::fp64_t DESERIALISATION_PER_CHAR_COEF;

  static const fixed_point::fp64_t DESERIALISATION_CONST_COEF;

  // Serialisation
  static const fixed_point::fp64_t SERIALISATION_PER_OP_COEF;
  static const fixed_point::fp64_t SERIALISATION_WEIGHT_SUM_COEF;
  static const fixed_point::fp64_t SERIALISATION_PADDED_WEIGHT_SUM_COEF;
  static const fixed_point::fp64_t SERIALISATION_CONST_COEF;

  static constexpr ChargeAmount CONSTANT_CHARGE{vm::COMPUTE_CHARGE_COST};

private:
  struct State
  {
    // Model
    DataType forward_pass_cost{"0.0"};
    DataType backward_pass_cost{"0.0"};
    DataType metrics_cost{"0.0"};

    // Optimiser
    SizeType weights_size_sum{0};
    SizeType weights_padded_size_sum{0};

    DataType optimiser_step_impact{"0"};

    SizeType last_layer_size{0};
    SizeType ops_count{0};

    // data
    SizeType subset_size{0};

    // serialization
    bool SerializeTo(serializers::MsgPackSerializer &buffer);
    bool DeserializeFrom(serializers::MsgPackSerializer &buffer);
  };

  VMObjectType &model_;
  State         state_;

  void CopyStateFrom(ModelEstimator const &src);

  static ChargeAmount MaximumCharge(std::string const &log_msg = "");

  static ChargeAmount ToChargeAmount(fixed_point::fp64_t const &val);
};

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
