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

#include "math/tensor.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/leaky_relu_op.hpp"
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
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;
  using WeightsInit  = fetch::ml::ops::WeightsInitialisation;

  explicit PRelu(std::uint64_t in, std::string const &name = "PRelu",
                 WeightsInit init_mode = WeightsInit::XAVIER_GLOROT)
  {
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});

    std::string alpha =
        this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Alpha", {});

    ArrayType alpha_data(std::vector<SizeType>({in, 1}));
    fetch::ml::ops::Weights<ArrayType>::Initialise(alpha_data, in, in, init_mode);

    this->SetInput(alpha, alpha_data);

    std::string output = this->template AddNode<fetch::ml::ops::LeakyReluOp<ArrayType>>(
        name + "_LeakyReluOp", {input, alpha});

    this->AddInputNode(input);
    this->SetOutputNode(output);
  }

  std::shared_ptr<SaveableParams<ArrayType>> GetOpSaveableParams()
  {
    throw std::runtime_error("This shouldn't be called!");
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    return {inputs.at(0).get().shape()};
  }

  static constexpr char const *DESCRIPTOR = "ParametricRelu";
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
