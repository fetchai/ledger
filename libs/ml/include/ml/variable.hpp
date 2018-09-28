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

#include "math/free_functions/free_functions.hpp"
#include "ml/Backprop.hpp"
#include "ml/session.hpp"

namespace fetch {
namespace ml {

template <typename ArrayType>
class Variable
{
public:
  using type               = ArrayType;
  using SelfType           = Variable<ArrayType>;
  using SessionType        = SessionManager<ArrayType, SelfType>;
  using function_signature = std::function<void(SelfType &)>;

  std::size_t           id;
  bool                  is_leaf = true;
  std::vector<SelfType> prev;
  function_signature    back_fn = nullptr;
  ArrayType             data;
  ArrayType             grad;

  std::vector<std::size_t> shape;
  bool                     initialised = false;

  Variable()
  {}
  Variable(SessionType &sess)
  {
    // set a distinct id for each element in the graph
    sess.RegisterVariable(*this);
  }
  Variable(std::size_t const &in_size, std::size_t const &out_size, SessionType &sess)
  {
    shape = {in_size, out_size};
    sess.RegisterVariable(*this);
  }
  Variable(std::vector<std::size_t> const &in_shape, SessionType &sess)
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
    back_fn = b_fn;
    is_leaf = false;
  }
  void Initialise(ArrayType in_data, function_signature b_fn = nullptr, bool in_is_leaf = true)
  {
    data = ArrayType(in_data);
    Setup(b_fn, in_is_leaf);
  }
  void Initialise(function_signature b_fn = nullptr, bool in_is_leaf = true)
  {
    data = ArrayType::UniformRandom(shape);
    Setup(b_fn, in_is_leaf);
  }

  static Variable Zeroes(std::vector<std::size_t> const &new_shape, SessionType &sess)
  {
    Variable ret{sess};
    ret.Initialise(ArrayType::Zeroes(new_shape));
    return ret;
  }
  static Variable Zeroes(std::size_t const &in_size, std::size_t const &out_size, SessionType &sess)
  {
    Variable ret{in_size, out_size};
    ret.Initialise(ArrayType::Zeroes(in_size, out_size));
    return ret;
  }

  bool operator==(Variable const &other) const
  {
    return this->id == other.id;
  }

  void Backward()
  {
    assert(initialised);
    assert(back_fn);
    Backprop(grad, back_fn, *this);
  }

  /**
   * interface for the data contained in the variable
   * @return
   */
  std::size_t size() const
  {
    return data.size();
  }

  /**
   * += operator
   * @param other
   * @return
   */
  void operator+=(ArrayType const &other)
  {
    fetch::math::Add(*this, other, *this);
  }
  void operator+=(typename ArrayType::type const &scalar)
  {
    fetch::math::Add(*this, scalar, *this);
  }
  SelfType operator+(ArrayType const &other)
  {
    fetch::math::Add(*this, other, *this);
    return *this;
  }
  SelfType operator+(typename ArrayType::type const &scalar)
  {
    fetch::math::Add(*this, scalar, *this);
    return *this;
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, typename ArrayType::type>::type &operator[](
      S const &i)
  {
    return this->data[i];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, typename ArrayType::type>::type const &
  operator[](S const &i) const
  {
    return this->data[i];
  }

  void Setup(function_signature b_fn = nullptr, bool in_is_leaf = true)
  {
    assert(b_fn || in_is_leaf);
    if (b_fn)
    {
      back_fn = b_fn;
    }
    is_leaf     = in_is_leaf;
    initialised = true;

    grad = ArrayType(data.shape());
    grad.data().SetAllZero();
  }
};

}  // namespace ml
}  // namespace fetch

// Doing this template specialization lets us find instances of Variable<T> in an unordered_set of
// them
namespace std {
template <typename ArrayType>
struct hash<fetch::ml::Variable<ArrayType>>
{
  std::size_t operator()(fetch::ml::Variable<ArrayType> const &v) const
  {
    return v.id;
  }
};
}  // namespace std