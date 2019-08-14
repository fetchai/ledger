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
  using SizeType      = typename TensorType::SizeType;
  using DataType      = typename TensorType::Type;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerFullyConnectedSaveableParams<TensorType>;

  using OpPtrType      = typename std::shared_ptr<fetch::ml::ops::Ops<TensorType>>;
  using GraphType      = typename fetch::ml::Graph<TensorType>;
  using GraphPtrType   = typename std::shared_ptr<GraphType>;
  using WeightsType    = typename fetch::ml::ops::Weights<TensorType>;
  using WeightsPtrType = typename std::shared_ptr<WeightsType>;

  FullyConnected() = default;

  /**
   * This initializer allows weight sharing to another fully connected layer through node interface
   * pointer.
   * @param target_node_ptr
   * @param in
   * @param out
   * @param activation_type
   * @param regulariser
   * @param regularisation_rate
   * @param init_mode
   */
  FullyConnected(OpPtrType target_node_ptr, SizeType in, SizeType out,
                 details::ActivationType       activation_type = details::ActivationType::NOTHING,
                 fetch::ml::RegularisationType regulariser = fetch::ml::RegularisationType::NONE,
                 DataType                      regularisation_rate = static_cast<DataType>(0),
                 WeightsInit init_mode = WeightsInit::XAVIER_GLOROT, bool time_distributed = false)
    : in_size_(in)
    , out_size_(out)
    , time_distributed_(time_distributed)
  {
    // since the weight is shared, we do not need to initialize the weight matrices.
    FETCH_UNUSED(init_mode);

    // setup overall architecture of the layer
    SetupArchitecture(activation_type, regulariser, regularisation_rate);

    // share weight with target_node
    ShareWeights(target_node_ptr);
  }

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
                 DataType                      regularisation_rate = static_cast<DataType>(0),
                 WeightsInit init_mode = WeightsInit::XAVIER_GLOROT, bool time_distributed = false)
    : in_size_(in)
    , out_size_(out)
    , time_distributed_(time_distributed)
  {
    // setup overall architecture of the model
    SetupArchitecture(activation_type, regulariser, regularisation_rate);

    // initialize the weights and bias with specified initialization method
    InitializeWeights(init_mode);
  }

  void InitializeWeights(WeightsInit init_mode)
  {
    // get correct name for the layer
    std::string name = GetName();

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
  }

  void ShareWeights(OpPtrType target_op_ptr)
  {
    // get correct name for the layer
    std::string name = GetName();

    // Get ptr to each weights layer
    GraphPtrType   target_graph_ptr        = std::dynamic_pointer_cast<GraphType>(target_op_ptr);
    OpPtrType      target_weights_node_ptr = target_graph_ptr->GetNode(name + "_Weights")->GetOp();
    WeightsPtrType target_weights_ptr =
        std::dynamic_pointer_cast<WeightsType>(target_weights_node_ptr);
    OpPtrType      target_bias_node_ptr = target_graph_ptr->GetNode(name + "_Bias")->GetOp();
    WeightsPtrType target_bias_ptr  = std::dynamic_pointer_cast<WeightsType>(target_bias_node_ptr);
    OpPtrType      weights_node_ptr = this->GetNode(name + "_Weights")->GetOp();
    WeightsPtrType weights_ptr      = std::dynamic_pointer_cast<WeightsType>(weights_node_ptr);
    OpPtrType      bias_node_ptr    = this->GetNode(name + "_Bias")->GetOp();
    WeightsPtrType bias_ptr         = std::dynamic_pointer_cast<WeightsType>(bias_node_ptr);

    // check if the shared weight is compatible with input shape
    auto w_s = target_weights_ptr->get_weights().shape();
    auto b_s = target_bias_ptr->get_weights().shape();
    ASSERT((w_s[0] == out_size_) && (w_s[1] == in_size_) && (b_s[0] == out_size_));
    if (time_distributed_)
    {
      assert(b_s.size() == 3);
    }
    else
    {
      assert(b_s.size() == 2);
    }

    // Share weights and parameter among these weights layers
    auto shared_weights = *(target_weights_ptr->GetShareableWeights());
    auto shared_bias    = *(target_bias_ptr->GetShareableWeights());
    this->SetInput(name + "_Weights", shared_weights);
    this->SetInput(name + "_Bias", shared_bias);
  }

  void SetupArchitecture(
      details::ActivationType       activation_type     = details::ActivationType::NOTHING,
      fetch::ml::RegularisationType regulariser         = fetch::ml::RegularisationType::NONE,
      DataType                      regularisation_rate = static_cast<DataType>(0))
  {
    // get correct name for the layer
    std::string name = GetName();

    // start to set up the structure
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

    // for non time distributed layer, flatten the input
    std::string flat_input = input;
    if (!time_distributed_)
    {
      flat_input =
          this->template AddNode<fetch::ml::ops::Flatten<TensorType>>(name + "_Flatten", {input});
    }

    std::string weights =
        this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights", {});
    std::string weights_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
        name + "_MatrixMultiply", {weights, flat_input});
    std::string bias =
        this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Bias", {});
    std::string output = this->template AddNode<fetch::ml::ops::Add<TensorType>>(
        name + "_Add", {weights_matmul, bias});

    output = fetch::ml::details::AddActivationNode<T>(activation_type, this, name + "_Activation",
                                                      output);

    this->AddInputNode(input);
    this->SetOutputNode(output);
    this->SetRegularisation(fetch::ml::details::CreateRegulariser<T>(regulariser),
                            regularisation_rate);
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
      for (size_t i = 0; i < inputs.front()->shape().size() - 1; i++)
      {
        total_in_size *= inputs.front()->shape(i);
      }
      assert(total_in_size == this->in_size_);
      return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 1)};
    }
    else
    {
      assert(inputs.front()->shape().size() == 3);
      assert(inputs.front()->shape(0) == in_size_);
      return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 2),
              inputs.front()->shape(inputs.front()->shape().size() - 1)};
    }
  }

  static constexpr OpType OpCode()
  {
    return fetch::ml::OpType::LAYER_FULLY_CONNECTED;
  }

  static constexpr char const *DESCRIPTOR = "FullyConnected";

private:
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
