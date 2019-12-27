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

#include "ml/core/subgraph.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/weights.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

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

  FullyConnected() = default;

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
                 WeightsInit init_mode = WeightsInit::XAVIER_GLOROT, bool time_distributed = false)
    : in_size_(in)
    , out_size_(out)
    , time_distributed_(time_distributed)
  {
    // get correct name for the layer
    std::string name = GetName();

    // start to set up the structure
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});
    nodes_.at(input)->SetDefaultOutputShape({in_size_, 1});

    // for non time distributed layer, flatten the input
    std::string flat_input = input;
    if (!time_distributed_)
    {
      flat_input =
          this->template AddNode<fetch::ml::ops::Flatten<TensorType>>(name + "_Flatten", {input});
      nodes_.at(flat_input)->SetDefaultOutputShape({out_size_, 1});
    }

    std::string weights =
        this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights", {});
    nodes_.at(weights)->SetDefaultOutputShape({out_size_, in_size_});
    std::string weights_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
        name + "_MatrixMultiply", {weights, flat_input});
    nodes_.at(weights_matmul)->SetDefaultOutputShape({out_size_, 1});
    std::string bias =
        this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Bias", {});
    nodes_.at(bias)->SetDefaultOutputShape({out_size_, 1});

    std::string output = this->template AddNode<fetch::ml::ops::Add<TensorType>>(
        name + "_Add", {weights_matmul, bias});
    nodes_.at(output)->SetDefaultOutputShape({out_size_, 1});

    output = fetch::ml::details::AddActivationNode<T>(activation_type, this, name + "_Activation",
                                                      output);
    nodes_.at(output)->SetDefaultOutputShape({out_size_, 1});

    this->AddInputNode(input);
    this->SetOutputNode(output);
    this->SetRegularisation(fetch::ml::details::CreateRegulariser<T>(regulariser),
                            regularisation_rate);

    // initialize weight with specified method
    TensorType weights_data(std::vector<SizeType>({out_size_, in_size_}));
    this->Initialise(weights_data, init_mode);
    this->SetInput(name + "_Weights", weights_data);

    // initialize bias with right shape and set to all zero
    TensorType bias_data;
    if (time_distributed_)
    {
      bias_data = TensorType(std::vector<SizeType>({out_size_, 1, 1}));
    }
    else
    {
      bias_data = TensorType(std::vector<SizeType>({out_size_, 1}));
    }
    this->SetInput(name + "_Bias", bias_data);
    this->Compile();
  }

  OpPtrType MakeSharedCopy(OpPtrType me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);  // used for compatability

    auto copyshare = std::make_shared<FullyConnected<TensorType>>();

    copyshare->time_distributed_ = time_distributed_;
    copyshare->in_size_          = in_size_;
    copyshare->out_size_         = out_size_;

    SubGraph<TensorType>::InsertSharedCopy(copyshare);

    return copyshare;
  }

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto ret = std::make_shared<SPType>();
    // get base class saveable params
    std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

    // assign base class saveable params to ret
    auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
    auto sg_ptr2 = std::static_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
    *sg_ptr2     = *sg_ptr1;

    // asign layer specific params
    ret->in_size          = in_size_;
    ret->out_size         = out_size_;
    ret->time_distributed = time_distributed_;

    return ret;
  }

  void SetOpSaveableParams(SPType const &sp)
  {
    // assign layer specific params
    in_size_          = sp.in_size;
    out_size_         = sp.out_size;
    time_distributed_ = sp.time_distributed;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    if (!time_distributed_)
    {
      SizeType total_in_size = 1;
      for (std::size_t i = 0; i < inputs.front()->shape().size() - 1; i++)
      {
        total_in_size *= inputs.front()->shape(i);
      }
      assert(total_in_size == this->in_size_);
      return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 1)};
    }

    assert(inputs.front()->shape().size() == 3);
    assert(inputs.front()->shape(0) == in_size_);
    return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 2),
            inputs.front()->shape(inputs.front()->shape().size() - 1)};
  }

  std::vector<SizeType> DefaultOutputShape() override
  {
    std::vector<SizeType> a{{out_size_}, {1}};
    SubGraph<T>::default_output_shape_ = a;
    return SubGraph<T>::default_output_shape_;
  }

  static constexpr OpType OpCode()
  {
    return fetch::ml::OpType::LAYER_FULLY_CONNECTED;
  }

  fetch::vm::ChargeAmount OpForwardCost(
      typename fetch::ml::ops::Ops<T>::VecShapesType const &input_shapes) override
  {
    FETCH_UNUSED(input_shapes);
    // Input shape is ignored because Dense layer knows all shapes already,
    // however it's a good idea to check if given shape matches to prevent crash.

    FETCH_LOG_INFO(DESCRIPTOR,
                   "Calculating forward pass cost: " + ops::Ops<T>::PrintMyOutputShape());

    // TODO(VH): stop on Placeholder, and calculate correct input shape for each
    // layer (assume Flatten, MatMul, Add, Activation are worth calculating)
    fetch::vm::ChargeAmount total_cost = 0;
    for (std::pair<std::string, typename Graph<T>::NodePtrType> const &node : nodes_)
    {
      total_cost += node.second->GetOp()->OpForwardCost({{out_size_, in_size_}});
    }

    FETCH_LOG_INFO(DESCRIPTOR, "  Cost calculated : " + std::to_string(total_cost) + "\n");
    return total_cost;
  }

  static constexpr char const *DESCRIPTOR = "FullyConnected";

private:
  using Graph<T>::nodes_;
  SizeType in_size_          = fetch::math::numeric_max<SizeType>();
  SizeType out_size_         = fetch::math::numeric_max<SizeType>();
  bool     time_distributed_ = false;

  void Initialise(TensorType &weights, WeightsInit init_mode)
  {
    fetch::ml::ops::Weights<TensorType>::Initialise(weights, in_size_, out_size_, init_mode);
  }

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
