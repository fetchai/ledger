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

//#include "math/free_functions/free_functions.hpp"
#include "ml/session.hpp"
#include <functional>
#include <memory>

namespace fetch {
namespace ml {

template <typename T>
class Variable : public std::enable_shared_from_this<Variable<T>>
{
public:
  using ArrayType          = T;
  using SelfType           = Variable<ArrayType>;
  using SessionType        = SessionManager<ArrayType, SelfType>;
  using function_signature = std::function<void(SelfType &)>;

  std::string           _variable_name = "";
  bool                  is_leaf = true;
  std::vector<std::shared_ptr<SelfType>> prev;
//  std::vector<SelfType> next;
  function_signature    back_fn = nullptr;
  function_signature    fwd_fn = nullptr;

  bool initialised = false;

  Variable() = default;

  Variable(SessionType &sess, std::string const &variable_name = "", function_signature const &b_fn = nullptr, bool in_is_leaf = true)
  {
    Setup(sess, b_fn, in_is_leaf, variable_name);
  }
//  Variable(SessionType &sess, ArrayType const &in_data, std::string variable_name = "", function_signature const &b_fn = nullptr, bool in_is_leaf = true)
//  {
//    _variable_name = variable_name;
//    sess.RegisterVariable(*this);
//
//    _data = ArrayType(in_data);
//    Setup(b_fn, in_is_leaf, variable_name);
//  }
  Variable(SessionType &sess, std::vector<std::size_t> in_shape, std::string const &variable_name = "", function_signature const &f_fn = nullptr, function_signature const &b_fn = nullptr, bool in_is_leaf = true)
  {
    _data = ArrayType(in_shape);
    Setup(sess, b_fn, in_is_leaf, variable_name, f_fn);
  }


  void SetData(ArrayType const &in_data)
  {
    //    shape = in_data.shape();
    _data = ArrayType(in_data);
  }

  static Variable Zeroes(std::vector<std::size_t> const &new_shape, SessionType &sess)
  {
    Variable ret{sess, ArrayType::Zeroes(new_shape)};
    return ret;
  }
  static Variable Zeroes(std::size_t const &in_size, std::size_t const &out_size, SessionType &sess)
  {
    std::vector<std::size_t> new_shape{in_size, out_size};
    Variable                 ret{sess, ArrayType::Zeroes(new_shape)};
    return ret;
  }

  void Forward()
  {
    assert(initialised);
    assert(fwd_fn);
    fwd_fn(*this);
  }

  void Backward()
  {
    assert(initialised);
    assert(back_fn);
    back_fn(*this);
  }

  /**
   * interface for the data contained in the variable
   * @return
   */
  std::size_t size() const
  {
    return _data.size();
  }
  std::vector<std::size_t> shape()
  {
    return _data.shape();
  }

  void Reshape(std::size_t const &i, std::size_t const &j)
  {
    _data.Reshape(i, j);
  }

  void GradientAdd(ArrayType const &other_grad)
  {
    _grad += other_grad;
  }

  void GradientValueAdd(std::size_t idx, typename ArrayType::type const &other_grad)
  {
    _grad[idx] += other_grad;
  }

  void GradientSetZero(std::size_t idx)
  {
    _grad[idx] = 0;
  }

  void GradientSetOne()
  {
    for (std::size_t i = 0; i < _grad.size(); ++i)
    {
      _grad[i] = 1;
    }
  }

  //
  //    /**
  //     * += operator
  //     * @param other
  //     * @return
  //     */
  //    void operator+=(ArrayType const &other)
  //    {
  //      fetch::math::Add(_data, other.data(), _data);
  //    }
  //    void operator+=(typename ArrayType::type const &scalar)
  //    {
  //      fetch::math::Add(*this, scalar, *this);
  //    }
  //    SelfType operator+(ArrayType const &other)
  //    {
  //      fetch::math::Add(*this, other, *this);
  //      return *this;
  //    }
  //    SelfType operator+(typename ArrayType::type const &scalar)
  //    {
  //      fetch::math::Add(*this, scalar, *this);
  //      return *this;
  //    }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, typename ArrayType::type>::type &operator[](
      S const &i)
  {
    return _data[i];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, typename ArrayType::type>::type const &
  operator[](S const &i) const
  {
    return _data[i];
  }

  bool operator<(SelfType const& other) const
  {
    return _id < other.id();
  }
  bool operator>(SelfType const& other) const
  {
    return _id > other.id();
  }
  bool operator==(SelfType const& other) const
  {
    return _id == other.id();
  }

  /* One-dimensional constant reference access function.
   * @param i is the index which is being accessed.
   *
   * Note this accessor is "slow" as it takes care that the developer
   * does not accidently enter the padded area of the memory.
   */
  typename ArrayType::type const &At(typename ArrayType::size_type const &i) const
  {
    return _data.At(i);
  }

  typename ArrayType::type &At(typename ArrayType::size_type const &i)
  {
    return _data.At(i);
  }
  typename ArrayType::type const &At(typename ArrayType::size_type const &i,
                                     typename ArrayType::size_type const &j) const
  {
    return _data.At(i, j);
  }
  typename ArrayType::type &At(typename ArrayType::size_type const &i,
                               typename ArrayType::size_type const &j)
  {
    return _data.At(i, j);
  }

  typename ArrayType::type const &Set(typename ArrayType::size_type const &n,
                                      typename ArrayType::type const &     v)
  {
    return _data.Set(n, v);
  }
  typename ArrayType::type const &Set(typename ArrayType::size_type const &i,
                                      typename ArrayType::size_type const &j,
                                      typename ArrayType::type const &     v)
  {
    return _data.Set(i, j, v);
  }

  /**
   * Apply a gradient update to the weights
   * @param lr
   */
  void GradientStep(typename ArrayType::type const& lr)
  {
    auto temp = Multiply(lr, _grad);
    Subtract(_data, temp, _data);
  }

  /**
   * Data accessor
   * @return
   */
  ArrayType const &data() const
  {
    return _data;
  }
  ArrayType &data()
  {
    return _data;
  }
  ArrayType const &grad() const
  {
    return _grad;
  }
  std::size_t &id()
  {
    return _id;
  }
  std::size_t const &id() const
  {
    return _id;
  }
  std::string const &variable_name() const
  {
    return _variable_name;
  }
  std::string &variable_name()
  {
    return _variable_name;
  }

private:
  ArrayType _data;
  ArrayType _grad;
  std::size_t           _id;

  void Setup(SessionType &sess, function_signature b_fn, bool in_is_leaf, std::string variable_name, function_signature f_fn = nullptr)
  {
    _variable_name = variable_name;

    sess.RegisterVariable(this->shared_from_this());

    assert(b_fn || in_is_leaf);
    back_fn = b_fn;
    if (f_fn)
    {
      fwd_fn = f_fn;
    }
    is_leaf     = in_is_leaf;
    initialised = true;

    _grad = ArrayType(_data.shape());
    _grad.data().SetAllZero();
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
    return v.id();
  }
};
}  // namespace std