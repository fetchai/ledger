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
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/layer_norm.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/weights.hpp"

#include <cmath>
#include <functional>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class LayerNorm : public SubGraph<T>
{
public:
  using TensorType    = T;
  using TensorPtrType = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using DataType      = typename TensorType::Type;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerLayerNormSaveableParams<T>;

  LayerNorm() = default;

  explicit LayerNorm(std::vector<SizeType> data_shape, SizeType axis = static_cast<SizeType>(0),
                     DataType epsilon = fetch::math::function_tolerance<DataType>())
    : data_shape_(std::move(data_shape))
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
        this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Gamma", {});
    std::string beta =
        this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Beta", {});

    // initialization: gamma to all 1, beta to all zero, and with corresponding shape
    std::vector<SizeType> weight_shape(data_shape_.size() + 1, 1);
    weight_shape[axis_] = data_shape_[axis_];
    TensorType gamma_data(weight_shape), beta_data(weight_shape);
    gamma_data.Fill(DataType{1});
    this->SetInput(gamma, gamma_data);
    this->SetInput(beta, beta_data);

    // setup input
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

    // do the normalization
    std::string normalized_output = this->template AddNode<fetch::ml::ops::LayerNorm<TensorType>>(
        name + "_LayerNorm", {input}, axis_, epsilon_);

    // do the rescaling
    std::string scaled_output = this->template AddNode<fetch::ml::ops::Multiply<TensorType>>(
        name + "_Gamma_Multiply", {normalized_output, gamma});

    // do the re-shifting
    std::string shifted_output = this->template AddNode<fetch::ml::ops::Add<TensorType>>(
        name + "_Beta_Addition", {scaled_output, beta});

    this->AddInputNode(input);
    this->SetOutputNode(shifted_output);

    this->Compile();
  }

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    // get all base classes saveable params
    std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

    auto ret = std::make_shared<SPType>();

    // copy subgraph saveable params over
    auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
    auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
    *sg_ptr2     = *sg_ptr1;

    // asign layer specific params
    ret->data_shape = data_shape_;
    ret->axis       = axis_;
    ret->epsilon    = epsilon_;

    return ret;
  }

  void SetOpSaveableParams(SPType const &sp)
  {
    // assign layer specific params
    data_shape_ = sp.data_shape;
    axis_       = sp.axis;
    epsilon_    = sp.epsilon;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return {inputs.at(0)->shape()};
  }

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_LAYER_NORM;
  }

  static constexpr char const *DESCRIPTOR = "LayerNorm";

private:
  std::vector<SizeType> data_shape_;
  SizeType              axis_{};
  DataType              epsilon_{};
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
