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

#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/weights.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/subgraph.hpp"

#include <cmath>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class BatchNorm : public SubGraph<T>
{
public:
  using TensorType    = T;
  using TensorPtrType = std::shared_ptr<TensorType>;
  using SizeType      = typename TensorType::SizeType;
  using DataType      = typename TensorType::Type;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;

  BatchNorm(SizeType data_size, DataType epsilon = fetch::math::numeric_lowest<DataType>(),
            DataType    regularisation_rate = static_cast<DataType>(0),
            WeightsInit init_mode           = WeightsInit::XAVIER_GLOROT)
    : data_size_(data_size)
  {
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

    // instantiate gamma (the multiplicative training component)
    std::string gamma = this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Gamma", {});
    ArrayType gamma_data({data_size_, 1});
    fetch::ml::ops::Weights<ArrayType>::Initialise(gamma_data, data_size_, init_mode);

    // instantiate beta (the additive training component)
    std::string beta = this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Beta", {});
    ArrayType beta_data({data_size_, 1});
    fetch::ml::ops::Weights<ArrayType>::Initialise(beta_data, data_size_, init_mode);


    std::string output = this->template AddNode<fetch::ml::ops::BatchNorm<TensorType>>(name + "_BatchNorm", {input, gamma, beta});

    this->AddInputNode(input);
    this->SetOutputNode(output);
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<TensorType const>> const &) const
  {
    return {inputs.at(0).get().shape()};
  }

  static constexpr char const *DESCRIPTOR = "BatchNorm";

private:
  SizeType data_size_;

};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
