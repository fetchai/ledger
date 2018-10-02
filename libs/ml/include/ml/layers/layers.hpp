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
//   limitations under the License.SessionManager
//
//------------------------------------------------------------------------------

#include "core/random/lcg.hpp"
#include "ml/ops/ops.hpp"
#include "ml/session.hpp"
#include "ml/variable.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <typename T>
class Layer
{
public:
  using ArrayType          = T;
  using SelfType           = Layer<ArrayType>;
  using VariableType       = Variable<ArrayType>;
  using SessionType        = SessionManager<ArrayType, VariableType>;
  using function_signature = std::function<void(SelfType &)>;

  std::size_t id;

  Layer(SessionType &in_sess, std::size_t const &in_size, std::size_t const &out_size)
  {
    _sess  = in_sess;
    _shape = {in_size, out_size};
    Setup();
  }
  Layer(SessionType &in_sess, std::vector<std::size_t> const &in_shape)
  {
    _sess  = in_sess;
    _shape = in_shape;
    Setup();
  }
  void Setup()
  {
    _weights = VariableType(_sess, ArrayType::UniformRandom(_shape));
    _biases  = VariableType(_sess, ArrayType::UniformRandom(1, _shape[1]));

    // TODO - implement correct weight initialisation as below
    //    std = 1.0/features_inp
    //    self.b = Variable(np.random.uniform(-std,std,features_out))
    //    self.w = Variable(np.random.uniform(-std,std,(features_inp,features_out)))
  }

  //  void AssignShape(std::vector<std::size_t> in_shape)
  //  {
  //    shape = in_shape;
  //  }
  //  void AssignBackFun(function_signature b_fn)
  //  {
  //    back_fn       = b_fn;
  //    this->is_leaf = false;
  //  }
  //
  //
  //  void Initialise(ArrayType in_data, function_signature b_fn = nullptr, bool in_is_leaf = true)
  //  {
  //    _weights = ArrayType(in_data);
  //    Setup(b_fn, in_is_leaf);
  //  }
  //  void Initialise(function_signature b_fn = nullptr, bool in_is_leaf = true)
  //  {
  //    _weights = ArrayType::UniformRandom(_shape);
  //    Setup(b_fn, in_is_leaf);
  //  }

  static Layer Zeroes(std::vector<std::size_t> const &new_shape, SessionType &sess)
  {
    Layer ret{sess};
    ret.Initialise(ArrayType::Zeroes(new_shape));
    return ret;
  }
  static Layer Zeroes(std::size_t const &in_size, std::size_t const &out_size, SessionType &sess)
  {
    Layer ret{in_size, out_size};
    ret.Initialise(ArrayType::Zeroes(in_size, out_size));
    return ret;
  }

  std::size_t InputSize()
  {
    return _weights.shape()[0];
  }

  std::size_t OutputSize()
  {
    return _weights.shape()[1];
  }

  //  void Backward()
  //  {
  //    assert(_weights->initialised);
  //    assert(_biases->initialised);
  //    assert(back_fn);
  //    Backprop(this->grad, back_fn, *this);
  //  }

  VariableType Forward(VariableType input, bool activate = true)
  {
    std::cout << "input shape[0]: " << input.shape()[0] << std::endl;
    std::cout << "input shape[1]: " << input.shape()[1] << std::endl;

    std::cout << "_weights shape[0]: " << _weights.shape()[0] << std::endl;
    std::cout << "_weights shape[1]: " << _weights.shape()[1] << std::endl;

    VariableType a_1 = fetch::ml::ops::Dot(input, _weights, _sess);
    //    fetch::ml::ops::Add(activations, _biases, activations);

    std::cout << "a_1 shape[0]: " << a_1.shape()[0] << std::endl;
    std::cout << "a_1 shape[1]: " << a_1.shape()[1] << std::endl;

    if (activate)
    {
      VariableType a_2 = fetch::ml::ops::Relu(a_1, _sess);
      std::cout << "a_2 shape[0]: " << a_2.shape()[0] << std::endl;
      std::cout << "a_2 shape[1]: " << a_2.shape()[1] << std::endl;
      VariableType a_3 = fetch::ml::ops::Sum(a_2, 1, _sess);

      std::cout << "a_3 shape[0]: " << a_3.shape()[0] << std::endl;
      std::cout << "a_3 shape[1]: " << a_3.shape()[1] << std::endl;

      return a_3;
    }
    else
    {
      return a_1;
    }
  }

  void ZeroGrads()
  {
    _weights.zero_grad();
    _biases.zero_grad();
  }

  void Step(typename ArrayType::type lr)
  {
    _weights.GradientStep(lr);
    _biases.GradientStep(lr);
  }

  VariableType weights()
  {
    return _weights;
  }
  VariableType biases()
  {
    return _biases;
  }
  std::vector<std::size_t> shape()
  {
    return _shape;
  }
  SessionType sess()
  {
    return _sess;
  }

private:
  std::vector<std::size_t> _shape;
  SessionType              _sess;
  VariableType             _weights;
  VariableType             _biases;
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