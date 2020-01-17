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

#include "ml/core/subgraph.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/weights.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class FullyConnected : public SubGraph<T>
{
public:
  using TensorType    = T;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using DataType      = typename TensorType::Type;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerFullyConnectedSaveableParams<TensorType>;

  using OpPtrType      = std::shared_ptr<fetch::ml::ops::Ops<TensorType>>;
  using GraphType      = fetch::ml::Graph<TensorType>;
  using GraphPtrType   = std::shared_ptr<GraphType>;
  using WeightsType    = fetch::ml::ops::Weights<TensorType>;
  using WeightsPtrType = std::shared_ptr<WeightsType>;

  /**
   * @brief FullyConnected default constructor without parameters is used to create an empty
   * object during deserialisation. After deserialisation the object is treated as an initialised
   * one.
   */
  FullyConnected()
    : SubGraph<T>()
    , is_initialised_(true)
  {}

  /**
   * Normal fully connected layer constructor
   * @param in
   * @param out
   * @param activation_type
   * @param regulariser
   * @param regularisation_rate
   * @param init_mode
   */
  FullyConnected(SizeType in, SizeType out,
                 details::ActivationType       activation_type = details::ActivationType::NOTHING,
                 fetch::ml::RegularisationType regulariser = fetch::ml::RegularisationType::NONE,
                 DataType                      regularisation_rate = DataType{0},
                 WeightsInit                   init_mode           = WeightsInit::XAVIER_GLOROT,
                 bool                          time_distributed    = !TIME_DISTRIBUTED);

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void SetOpSaveableParams(SPType const &sp);

  OpPtrType MakeSharedCopy(OpPtrType me) override;

  void CompleteInitialisation() override;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  math::SizeVector ComputeBatchOutputShape(
      std::vector<math::SizeVector> const &input_shapes) override;

  static constexpr OpType OpCode()
  {
    return fetch::ml::OpType::LAYER_FULLY_CONNECTED;
  }

  static constexpr char const *DESCRIPTOR = "FullyConnected";

  inline OpType OperationType() const override
  {
    return this->OpCode();
  }
  inline char const *Descriptor() const override
  {
    return DESCRIPTOR;
  }

  static constexpr SizeType AUTODETECT_INPUT_SHAPE = 0;
  static constexpr bool     TIME_DISTRIBUTED       = true;

private:
  // Saveable params
  SizeType in_size_          = fetch::math::numeric_max<SizeType>();
  SizeType out_size_         = fetch::math::numeric_max<SizeType>();
  bool     time_distributed_ = false;

  // Non-saveable params
  bool                          is_initialised_ = false;
  std::string                   input_;
  std::string                   flattened_input_;
  std::string                   weights_;
  std::string                   bias_;
  std::string                   output_;
  fetch::ml::RegularisationType regulariser_;
  DataType                      regularisation_rate_;
  WeightsInit                   init_mode_;

  std::string GetName()
  {
    std::string name = DESCRIPTOR;
    if (time_distributed_)
    {
      name = "TimeDistributed_" + name;
    }
    return name;
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
