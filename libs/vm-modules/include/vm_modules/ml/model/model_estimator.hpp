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

  SizeType GetPaddedSizesSum();
  SizeType GetSizesSum();
  SizeType GetOpsCount();
  DataType GetForwardCost();

  // AddLayerFORWARD_DENSE_INPUT_COEF
  static constexpr DataType ADD_DENSE_INPUT_COEF()
  {
    return DataType(0.111111111111111);
  };
  static constexpr DataType ADD_DENSE_OUTPUT_COEF()
  {
    return DataType(0.043478260869565);
  };
  static constexpr DataType ADD_DENSE_QUAD_COEF()
  {
    return DataType(0.013513513513514);
  };
  static constexpr DataType ADD_DENSE_CONST_COEF()
  {
    return DataType(52.0);
  };

  // Compile
  static constexpr DataType ADAM_PADDED_WEIGHTS_SIZE_COEF()
  {
    return DataType(0.014285714285714);
  };
  static constexpr DataType ADAM_WEIGHTS_SIZE_COEF()
  {
    return DataType(0.017857142857143);
  };
  static constexpr DataType ADAM_STEP_IMPACT_COEF()
  {
    return DataType(0.017857142857143);
  };

  static constexpr DataType SGD_PADDED_WEIGHTS_SIZE_COEF()
  {
    return DataType(0.014285714285714);
  };
  static constexpr DataType SGD_WEIGHTS_SIZE_COEF()
  {
    return DataType(0.017857142857143);
  };
  static constexpr DataType SGD_STEP_IMPACT_COEF()
  {
    return DataType(0.017857142857143);
  };

  static constexpr DataType COMPILE_CONST_COEF()
  {
    return DataType(80);
  };

  // Forward
  static constexpr DataType FORWARD_DENSE_INPUT_COEF()
  {
    return DataType(0.142857142857143);
  };
  static constexpr DataType FORWARD_DENSE_OUTPUT_COEF()
  {
    return DataType(0.037037037037037);
  };
  static constexpr DataType FORWARD_DENSE_QUAD_COEF()
  {
    return DataType(0.013157894736842);
  };

  static constexpr DataType RELU_FORWARD_IMPACT()
  {
    return DataType(0.003333333333333);
  };
  static constexpr DataType MSE_FORWARD_IMPACT()
  {
    return DataType(0.003333333333333);
  };
  static constexpr DataType CEL_FORWARD_IMPACT()
  {
    return DataType(0.003333333333333);
  };

  // Backward
  static constexpr DataType BACKWARD_DENSE_INPUT_COEF()
  {
    return DataType(0.142857142857143);
  };
  static constexpr DataType BACKWARD_DENSE_OUTPUT_COEF()
  {
    return DataType(0.037037037037037);
  };
  static constexpr DataType BACKWARD_DENSE_QUAD_COEF()
  {
    return DataType(0.013157894736842);
  };
  static constexpr DataType RELU_BACKWARD_IMPACT()
  {
    return DataType(0.003333333333333);
  };
  static constexpr DataType MSE_BACKWARD_IMPACT()
  {
    return DataType(0.003333333333333);
  };
  static constexpr DataType CEL_BACKWARD_IMPACT()
  {
    return DataType(0.003333333333333);
  };

  // Predict
  static constexpr DataType PREDICT_BATCH_LAYER_COEF()
  {
    return DataType(0.3);
  };
  static constexpr DataType PREDICT_CONST_COEF()
  {
    return DataType(40.0);
  };

  // Fit
  static constexpr DataType BACKWARD_BATCH_LAYER_COEF()
  {
    return DataType(0.3);
  };

  static constexpr DataType BACKWARD_PER_BATCH_COEF()
  {
    return DataType(0.3);
  };

  static constexpr DataType FIT_CONST_COEF()
  {
    return DataType(40.0);
  };

  // Deserialisation
  static constexpr DataType DESERIALISATION_PER_CHAR_COEF()
  {
    return DataType(0.010416666666667);
  };

  static constexpr DataType DESERIALISATION_CONST_COEF()
  {
    return DataType(100.0);
  };

  // Serialisation
  static constexpr DataType SERIALISATION_PER_OP_COEF()
  {
    return DataType(139);
  };

  static constexpr DataType SERIALISATION_WEIGHT_SUM_COEF()
  {
    return DataType(0.05292996);
  };

  static constexpr DataType SERIALISATION_PADDED_WEIGHT_SUM_COEF()
  {
    return DataType(0.2);
  };

  static constexpr DataType SERIALISATION_CONST_COEF()
  {
    return DataType(210);
  };

  static constexpr ChargeAmount constant_charge{vm::CHARGE_UNIT};

private:
  struct State
  {
    // Model
    DataType forward_pass_cost{0.0};
    DataType backward_pass_cost{0.0};

    // Optimiser
    SizeType weights_size_sum{0};
    SizeType weights_padded_size_sum{0};

    DataType optimiser_step_impact{0};

    SizeType last_layer_size{0};
    SizeType ops_count{0};

    // serialization
    bool SerializeTo(serializers::MsgPackSerializer &buffer);
    bool DeserializeFrom(serializers::MsgPackSerializer &buffer);
  };

  VMObjectType &model_;
  State         state_;

  void copy_state_from(ModelEstimator const &src);

  static ChargeAmount infinite_charge(std::string const &log_msg = "");
};

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
