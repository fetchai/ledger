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

namespace fetch {
namespace ml {
namespace layers {

template <typename T>
class Layer
{
public:
  using ArrayType         = T;
  using SelfType          = Layer<ArrayType>;
  using VariableType      = fetch::ml::Variable<ArrayType>;
  using VariablePtrType   = std::shared_ptr<SelfType>;
  using FunctionSignature = std::function<void(VariablePtrType)>;
  using SizeType          = std::size_t;
  using ShapeType         = std::vector<std::size_t>;

  std::size_t id;

  void Initialise(ShapeType const &shape, VariablePtrType weights, VariablePtrType biases)
  {
    shape_ = shape;

    weights_ = weights;
    biases_  = biases;

    weights_->data() = ArrayType::UniformRandom(shape_);
    biases_->data()  = ArrayType::UniformRandom(1, shape_[1]);

    //    weights_->data().Copy(ArrayType::UniformRandom(shape_));
    //    biases_->data().Copy(ArrayType::UniformRandom(1, shape_[1]));

    //    weights_ = VariableType(_sess, ArrayType::UniformRandom(shape_));
    //    biases_  = VariableType(_sess, ArrayType::UniformRandom(1, shape_[1]));

    // (TODO private 273)
    //    something like the following would be the simplest first step
    //    std = 1.0/features_inp
    //    self.b = Variable(np.random.uniform(-std,std,features_out))
    //    self.w = Variable(np.random.uniform(-std,std,(features_inp,features_out)))
  }

  //  void AssignShape(std::vector<std::size_t> inshape_)
  //  {
  //    shape = inshape_;
  //  }
  //  void AssignBackFun(FunctionSignature b_fn)
  //  {
  //    back_fn       = b_fn;
  //    this->is_leaf = false;
  //  }
  //
  //
  //  void Initialise(ArrayType in_data, FunctionSignature b_fn = nullptr, bool in_is_leaf = true)
  //  {
  //    weights_ = ArrayType(in_data);
  //    Setup(b_fn, in_is_leaf);
  //  }
  //  void Initialise(FunctionSignature b_fn = nullptr, bool in_is_leaf = true)
  //  {
  //    weights_ = ArrayType::UniformRandom(shape_);
  //    Setup(b_fn, in_is_leaf);
  //  }

  std::size_t InputSize()
  {
    return weights_.shape()[0];
  }

  std::size_t OutputSize()
  {
    return weights_.shape()[1];
  }

  //  void Backward()
  //  {
  //    assert(weights_->initialised);
  //    assert(biases_->initialised);
  //    assert(back_fn);
  //    Backprop(this->grad, back_fn, *this);
  //  }

  VariableType Forward()
  {

    weights_.Forward();
    biases_.Forward();

    //    VariableType a_1 = fetch::ml::ops::Dot(input, weights_, _sess);
    //    //    fetch::ml::ops::Add(activations, biases_, activations);
    //
    //
    //    if (activate)
    //    {
    //      VariableType a_2 = fetch::ml::ops::Relu(a_1, _sess);
    //      VariableType a_3 = fetch::ml::ops::Sum(a_2, 1, _sess);
    //
    //      return a_3;
    //    }
    //    else
    //    {
    //      return a_1;
    //    }
  }

  void ZeroGrads()
  {
    weights_.zero_grad();
    biases_.zero_grad();
  }

  void Step(typename ArrayType::type lr)
  {
    weights_.GradientStep(lr);
    biases_.GradientStep(lr);
  }

  VariableType weights()
  {
    return weights_;
  }
  VariableType biases()
  {
    return biases_;
  }
  std::vector<std::size_t> shape()
  {
    return shape_;
  }

private:
  std::vector<std::size_t> shape_;
  VariablePtrType          weights_;
  VariablePtrType          biases_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch

//// Doing this template specialization lets us find instances of Variable<T> in an unordered_set of
//// them
// namespace std {
// template <typename ArrayType>
// struct hash<fetch::ml::layers::Layer<ArrayType>>
//{
//  std::size_t operator()(fetch::ml::layers::Layer<ArrayType> const &v) const
//  {
//    //    return &v.prev;
//    return v.id;
//  }
//};
//}  // namespace std