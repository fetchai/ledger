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
#include "ml/Backprop.hpp"
#include "ml/session.hpp"
#include "ml/variable.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <typename T>
class Layer : public Variable<T>
{
public:
  using ArrayType          = T;
  using SelfType           = Layer<ArrayType>;
  using SessionType        = SessionManager<ArrayType, SelfType>;
  using function_signature = std::function<void(SelfType &)>;

  std::size_t           id;
  std::vector<SelfType> prev;
  function_signature    back_fn;

  std::vector<std::size_t> shape;

  Layer() = default;
  Layer(SessionType &sess)
  {
    // set a distinct id for each element in the graph
    sess.RegisterVariable(*this);
  }
  Layer(std::size_t const &in_size, std::size_t const &out_size, SessionType &sess)
  {
    shape = {in_size, out_size};
    sess.RegisterVariable(*this);
  }
  Layer(std::vector<std::size_t> const &in_shape, SessionType &sess)
  {
    shape = in_shape;
    sess.RegisterVariable(*this);
  }

  void AssignShape(std::vector<std::size_t> in_shape)
  {
    shape = in_shape;
  }
  void AssignBackFun(function_signature b_fn)
  {
    back_fn       = b_fn;
    this->is_leaf = false;
  }
  void Initialise(ArrayType in_data, function_signature b_fn = nullptr, bool in_is_leaf = true)
  {
    this->data = ArrayType(in_data);
    Setup(b_fn, in_is_leaf);
  }
  void Initialise(function_signature b_fn = nullptr, bool in_is_leaf = true)
  {
    this->data = ArrayType::UniformRandom(shape);
    Setup(b_fn, in_is_leaf);
  }

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

  bool operator==(Layer const &other) const
  {
    return this->id == other.id;
  }

  std::size_t InputSize()
  {
    return this->data.shape()[0];
  }

  std::size_t OutputSize()
  {
    return this->data.shape()[1];
  }

  void Setup(function_signature b_fn = nullptr, bool in_is_leaf = true)
  {
    assert(b_fn || in_is_leaf);
    if (b_fn)
    {
      back_fn = b_fn;
    }
    this->is_leaf     = in_is_leaf;
    this->initialised = true;

    this->grad = ArrayType(this->data.shape());
    this->grad.data().SetAllZero();
  }

  void Backward()
  {
    assert(this->initialised);
    assert(back_fn);
    Backprop(this->grad, back_fn, *this);
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch

// Doing this template specialization lets us find instances of Variable<T> in an unordered_set of
// them
namespace std {
template <typename ArrayType>
struct hash<fetch::ml::layers::Layer<ArrayType>>
{
  std::size_t operator()(fetch::ml::layers::Layer<ArrayType> const &v) const
  {
    //    return &v.prev;
    return v.id;
  }
};
}  // namespace std