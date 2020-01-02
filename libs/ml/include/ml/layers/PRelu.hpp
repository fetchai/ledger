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

#include "math/tensor/tensor.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/prelu_op.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class PRelu : public SubGraph<T>
{
public:
  using TensorType    = T;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerPReluSaveableParams<TensorType>;

  PRelu() = default;

  explicit PRelu(uint64_t in, std::string const &name = "PRelu",
                 WeightsInit init_mode = WeightsInit::XAVIER_GLOROT)
  {
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

    std::string alpha =
        this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Alpha", {});

    TensorType alpha_data(std::vector<SizeType>({in, 1}));
    fetch::ml::ops::Weights<TensorType>::Initialise(alpha_data, in, in, init_mode);

    this->SetInput(alpha, alpha_data);

    std::string output = this->template AddNode<fetch::ml::ops::PReluOp<TensorType>>(
        name + "_PReluOp", {input, alpha});

    this->AddInputNode(input);
    this->SetOutputNode(output);

    this->Compile();
  }

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    // get base class saveable params
    std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

    auto ret = std::make_shared<SPType>();

    // copy subgraph saveable params over
    auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
    auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
    *sg_ptr2     = *sg_ptr1;

    return ret;
  }

  void SetOpSaveableParams(SPType const &sp)
  {
    FETCH_UNUSED(sp);
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return {inputs.at(0)->shape()};
  }

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_PRELU;
  }

  static constexpr char const *DESCRIPTOR = "ParametricRelu";
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
