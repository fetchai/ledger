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

  BatchNorm(SizeVector data_shape, DataType epsilon = fetch::math::numeric_lowest<DataType>(),
            DataType    regularisation_rate = static_cast<DataType>(0),
            WeightsInit init_mode           = WeightsInit::XAVIER_GLOROT)
    : data_shape_(data_shape)
  {
    data_shape.emplace_back(static_cast<SizeType>(1));
    SizeType data_size = fetch::math::Product(data_shape);

    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

    //    std::string gamma =
    //        this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Gamma", {});
    //    TensorType gamma_data(data_shape);
    //    fetch::ml::ops::Weights<TensorType>::Initialise(gamma_data, data_size, 1, init_mode);
    //
    //    std::string beta =
    //        this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Beta", {});
    //    TensorType beta_data(data_shape);
    //    fetch::ml::ops::Weights<TensorType>::Initialise(beta_data, data_size, 1, init_mode);
    //
    //    this->SetInput(gamma, gamma_data);
    //    this->SetInput(beta, beta_data);

    std::string mean_normalise_output =
        this->template AddNode<fetch::ml::ops::BatchNorm<TensorType>>(name + "_MeanNormalise",
                                                                      {input});

    this->AddInputNode(input);
    this->SetOutputNode(output);
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<TensorType const>> const &) const
  {
    return {this->data_size_, 1};
  }

  static constexpr char const *DESCRIPTOR = "BatchNorm";

private:
  SizeType data_size_;

  void Initialise(TensorType &weights, WeightsInit init_mode)
  {
    fetch::ml::ops::Weights<TensorType>::Initialise(weights, in_size_, data_size_, init_mode);
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
