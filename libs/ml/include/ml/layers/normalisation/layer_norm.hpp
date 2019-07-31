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
#include "ml/ops/layer_norm.hpp"
#include "ml/ops/multiply.hpp"
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
class LayerNorm : public SubGraph<T>
{
public:
  using ArrayType     = T;
  using TensorPtrType = std::shared_ptr<ArrayType>;
  using SizeType      = typename ArrayType::SizeType;
  using DataType      = typename ArrayType::Type;
  using VecTensorType = typename SubGraph<T>::VecTensorType;

  LayerNorm(std::vector<SizeType> const &data_shape, SizeType axis = static_cast<SizeType>(0),
            DataType epsilon = fetch::math::numeric_lowest<DataType>())
    : data_shape_(data_shape)
    , axis_(axis)
    , epsilon_(epsilon)
  {
    // the data_shape is the shape of the data without including the batch dims
    // make sure we do not do normalization along the batch dims
    ASSERT(axis_ != data_shape_.size());

    // Due to constrain in Add and Multiply layer, we do not support data of more than 3 dims
    ASSERT(data_shape_.size() <= 2);

    std::string name = DESCRIPTOR;

    // instantiate gamma and beta (the multiplicative training component)
    std::string gamma =
        this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Gamma", {});
    std::string beta =
        this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Beta", {});
    ArrayType gamma_data, beta_data;

    // initialization: gamma to all 1, beta to all zero, and with corresponding shape
    std::vector<SizeType> weight_shape(data_shape_.size() + 1, 1);
    weight_shape[axis_] = data_shape_[axis_];
    gamma_data.Reshape(weight_shape);
    beta_data.Reshape(weight_shape);
    gamma_data.Fill(static_cast<DataType>(1));
    this->SetInput(gamma, gamma_data);
    this->SetInput(beta, beta_data);

    // setup input
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});

    // do the normalization
    std::string normalized_output = this->template AddNode<fetch::ml::ops::LayerNorm<ArrayType>>(
        name + "_LayerNorm", {input}, axis_);

    // do the rescaling
    std::string scaled_output = this->template AddNode<fetch::ml::ops::Multiply<ArrayType>>(
        name + "_Gamma_Multiply", {normalized_output, gamma});

    // do the re-shifting
    std::string shifted_output = this->template AddNode<fetch::ml::ops::Add<ArrayType>>(
        name + "_Beta_Addition", {normalized_output, beta});

    this->AddInputNode(input);
    this->SetOutputNode(shifted_output);
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return {inputs.at(0)->shape()};
  }

  static constexpr char const *DESCRIPTOR = "LayerNorm";

private:
  std::vector<SizeType> data_shape_;
  SizeType              axis_;
  DataType              epsilon_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
