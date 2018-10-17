#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/random/lcg.hpp"
#include "ml/ops/ops.hpp"
#include "ml/variable.hpp"
#include <cmath>
#include <random>

namespace fetch {
namespace ml {
namespace layers {

template <typename T>
class Layer
{
public:
  using ArrayType         = T;
  using SelfType          = Layer<ArrayType>;
  using SelfPtrType       = std::shared_ptr<SelfType>;
  using VariableType      = fetch::ml::Variable<ArrayType>;
  using VariablePtrType   = std::shared_ptr<VariableType>;
  using FunctionSignature = std::function<void(VariablePtrType)>;
  using SizeType          = std::size_t;
  using ShapeType         = std::vector<std::size_t>;

  std::size_t id;

  void Initialise(ShapeType const &shape, VariablePtrType weights, VariablePtrType biases)
  {
    shape_ = shape;

    weights_ = weights;
    // (TODO private 273)

    biases_ = biases;

    std::random_device         rd{};
    std::mt19937               gen{rd()};
    auto                       in  = typename ArrayType::Type(shape_[0]);
    auto                       out = typename ArrayType::Type(shape_[1]);
    std::normal_distribution<> d{0.0, fetch::math::Divide(2.0, in + out)};
    for (std::size_t i = 0; i < weights_->data().size(); ++i)
    {
      weights_->data().Set(i, d(gen));
    }
    for (std::size_t i = 0; i < biases_->data().size(); ++i)
    {
      biases_->data().Set(i, 0);
    }
  }

  void ActivationSetup(std::string activate)
  {
    activate_ = activate;
  }
  void BiasesSetup(bool has_biases)
  {
    has_biases_ = has_biases;
  }

  std::size_t InputSize()
  {
    return weights_->shape()[0];
  }

  std::size_t OutputSize()
  {
    return weights_->shape()[1];
  }

  VariableType Forward()
  {

    weights_.Forward(prev_);
    biases_.Forward(prev_);
  }

  void ZeroGrads()
  {
    weights_.zero_grad();
    biases_.zero_grad();
  }

  void Step(typename ArrayType::Type lr)
  {
    weights_.GradientStep(lr);
    biases_.GradientStep(lr);
  }

  VariablePtrType weights()
  {
    return weights_;
  }

  VariablePtrType biases()
  {
    return biases_;
  }

  std::vector<std::size_t> shape()
  {
    return shape_;
  }

  VariablePtrType dot_hidden()
  {
    return dot_;
  }
  VariablePtrType add_hidden()
  {
    return add_;
  }
  VariablePtrType output()
  {
    return output_;
  }

  template <typename SessionType>
  void SetInput(VariablePtrType input, SessionType &sess)
  {
    prev_ = input;
    if (has_biases_)
    {
      if (activate_ != "")
      {
        dot_    = fetch::ml::ops::Dot(input, weights_, sess);
        add_    = fetch::ml::ops::AddBroadcast(dot_, biases_, sess);
        output_ = Activate(add_, sess);
      }
      else
      {
        dot_    = fetch::ml::ops::Dot(input, weights_, sess);
        output_ = fetch::ml::ops::AddBroadcast(dot_, biases_, sess);
      }
    }
    else
    {
      if (activate_ != "")
      {
        dot_    = fetch::ml::ops::Dot(input, weights_, sess);
        output_ = Activate(dot_, sess);
      }
      else
      {
        output_ = fetch::ml::ops::Dot(input, weights_, sess);
      }
    }
  }

  template <typename SessionType>
  VariablePtrType Activate(VariablePtrType hidden_states, SessionType &sess)
  {

    if (activate_ == "LeakyRelu")
    {
      return fetch::ml::ops::LeakyRelu(hidden_states, sess);
    }
    else if (activate_ == "Relu")
    {
      return fetch::ml::ops::Relu(hidden_states, sess);
    }
    else if (activate_ == "Sigmoid")
    {
      return fetch::ml::ops::Sigmoid(hidden_states, sess);
    }
    else if (activate_ == "Softmax")
    {
      return fetch::ml::ops::Softmax(hidden_states, sess);
    }
    else if (activate_ == "")
    {
      return hidden_states;
    }
    else
    {
      std::cout << "unspecified activation type" << std::endl;
      assert(false);
      return hidden_states;
    }
  }

private:
  std::vector<std::size_t> shape_;
  VariablePtrType          weights_;
  VariablePtrType          biases_;
  VariablePtrType          prev_;
  VariablePtrType          dot_;
  VariablePtrType          add_;
  VariablePtrType          output_;
  bool                     has_biases_;
  std::string              activate_ = "LeakyRelu";
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch