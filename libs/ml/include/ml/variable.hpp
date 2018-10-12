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
#include <functional>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {

template <typename T>
class Variable
{
public:
  using ArrayType         = T;
  using SelfType          = Variable<ArrayType>;
  using SelfPtrType       = std::shared_ptr<SelfType>;
  using FunctionSignature = std::function<void(SelfPtrType)>;
  using ShapeType         = std::vector<std::size_t>;

  bool                     initialised = false;
  std::vector<SelfPtrType> prev;

  Variable() = default;

  void SetVariableName(std::string const &variable_name)
  {
    variable_name_ = variable_name;
  }

  void SetBackwardFunction(FunctionSignature b_fn)
  {
    b_fn_ = b_fn;
  }

  void SetForwardFunction(FunctionSignature f_fn)
  {
    f_fn_ = f_fn;
  }

  void SetIsLeaf(bool is_leaf, bool requires_grad = false)
  {
    is_leaf_ = is_leaf;

    // All non-leafs require gradients
    if (!is_leaf)
    {
      requires_grad_ = true;
    }

    // leafs may or may not require gradients
    else
    {
      requires_grad_ = requires_grad;
    }
  }

  void SetData(ArrayType const &indata_)
  {
    data_ = ArrayType(indata_);
  }

  void Forward(SelfPtrType ptr)
  {
    assert(initialised);
    assert(f_fn_);
    f_fn_(ptr);
  }

  void Backward(SelfPtrType ptr, typename ArrayType::Type lambda = 0.0)
  {
    assert(initialised);
    assert(b_fn_);
    b_fn_(ptr);
  }

  /**
   * interface for the data contained in the variable
   * @return
   */
  std::size_t size() const
  {
    return data_.size();
  }
  std::vector<std::size_t> shape()
  {
    return data_.shape();
  }

  void Reshape(std::size_t const &i, std::size_t const &j)
  {
    data_.Reshape(i, j);
  }

  void GradientAdd(ArrayType const &othergrad_)
  {
    grad_ += othergrad_;
  }

  void GradientValueAdd(std::size_t idx, typename ArrayType::Type const &othergrad_)
  {
    grad_[idx] += othergrad_;
  }

  void GradientValueAdd(std::size_t i, std::size_t j, typename ArrayType::Type const &othergrad_)
  {
    grad_.Set(i, j, grad_.At(i, j) + othergrad_);
  }

  void GradientSetZero(std::size_t idx)
  {
    grad_[idx] = 0;
  }

  void GradientSetOne()
  {
    for (std::size_t i = 0; i < grad_.size(); ++i)
    {
      grad_[i] = 1;
    }
  }

  void GradientSetVal(typename ArrayType::Type const &othergrad_)
  {
    for (std::size_t i = 0; i < grad_.size(); ++i)
    {
      grad_[i] = othergrad_;
    }
  }

  void InitialiseGradients(std::vector<std::size_t> &grad_shape)
  {
    //    std::vector<std::size_t> new_shape{1, data().shape()[1]};
    //    std::vector<std::size_t> new_shape{data().shape()};
    grad_ = ArrayType(grad_shape);
    //    grad_ = ArrayType(new_shape);
    ClearGradients();
  }
  void ClearGradients()
  {
    grad_.SetAllZero();
  }

  //
  //    /**
  //     * += operator
  //     * @param other
  //     * @return
  //     */
  //    void operator+=(ArrayType const &other)
  //    {
  //      fetch::math::Add(data_, other.data(), data_);
  //    }
  //    void operator+=(typename ArrayType::Type const &scalar)
  //    {
  //      fetch::math::Add(*this, scalar, *this);
  //    }
  //    SelfType operator+(ArrayType const &other)
  //    {
  //      fetch::math::Add(*this, other, *this);
  //      return *this;
  //    }
  //    SelfType operator+(typename ArrayType::Type const &scalar)
  //    {
  //      fetch::math::Add(*this, scalar, *this);
  //      return *this;
  //    }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, typename ArrayType::Type>::type &operator[](
      S const &i)
  {
    return data_[i];
  }

  template <typename S>
  typename std::enable_if<std::is_integral<S>::value, typename ArrayType::Type>::type const &
  operator[](S const &i) const
  {
    return data_[i];
  }

  bool operator<(SelfType const &other) const
  {
    return id_ < other.id();
  }
  bool operator>(SelfType const &other) const
  {
    return id_ > other.id();
  }
  bool operator==(SelfType const &other) const
  {
    return id_ == other.id();
  }

  /* One-dimensional constant reference access function.
   * @param i is the index which is being accessed.
   *
   * Note this accessor is "slow" as it takes care that the developer
   * does not accidently enter the padded area of the memory.
   */
  typename ArrayType::Type const &At(typename ArrayType::size_type const &i) const
  {
    return data_.At(i);
  }

  typename ArrayType::Type &At(typename ArrayType::size_type const &i)
  {
    return data_.At(i);
  }
  typename ArrayType::Type const &At(typename ArrayType::size_type const &i,
                                     typename ArrayType::size_type const &j) const
  {
    return data_.At(i, j);
  }
  typename ArrayType::Type &At(typename ArrayType::size_type const &i,
                               typename ArrayType::size_type const &j)
  {
    return data_.At(i, j);
  }

  typename ArrayType::Type const &Set(typename ArrayType::size_type const &n,
                                      typename ArrayType::Type const &     v)
  {
    return data_.Set(n, v);
  }
  typename ArrayType::Type const &Set(typename ArrayType::size_type const &i,
                                      typename ArrayType::size_type const &j,
                                      typename ArrayType::Type const &     v)
  {
    return data_.Set(i, j, v);
  }

  /**
   * Apply a gradient update to the weights
   * @param lr
   */
  void GradientStep(typename ArrayType::Type const &lr,
                    typename ArrayType::Type const &gradient_clip)
  {
    if (gradient_clip < 0)
    {
      Subtract(data_, Multiply(lr, grad_), data_);
    }
    else
    {
      auto l2_norm = fetch::math::L2Norm(grad_);
      auto delta   = fetch::math::Divide(grad_, fetch::math::Max(l2_norm, gradient_clip));
      delta        = Multiply(lr, delta);
      Subtract(data_, delta, data_);
    }
  }

  /**
   * Data accessor
   * @return
   */
  ArrayType const &data() const
  {
    return data_;
  }
  ArrayType &data()
  {
    return data_;
  }
  ArrayType const &grad() const
  {
    return grad_;
  }
  std::size_t &id()
  {
    return id_;
  }
  std::size_t const &id() const
  {
    return id_;
  }
  std::string const &variable_name() const
  {
    return variable_name_;
  }
  std::string &variable_name()
  {
    return variable_name_;
  }

  bool const &is_leaf()
  {
    return is_leaf_;
  }

  bool const &requires_grad()
  {
    return requires_grad_;
  }

private:
  ArrayType   data_;
  ArrayType   grad_;
  std::size_t id_;

  std::string       variable_name_ = "";
  bool              is_leaf_       = true;
  bool              requires_grad_ = false;
  FunctionSignature b_fn_          = nullptr;
  FunctionSignature f_fn_          = nullptr;
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