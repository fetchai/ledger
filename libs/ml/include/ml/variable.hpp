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

namespace fetch {
namespace ml {

std::size_t variable_counter = 0;

template <typename ArrayType>
class Variable
{
public:
  using function_signature = std::function<void(Variable<ArrayType> &)>;
  //  using function_signature = std::function<void(ArrayType&, std::vector<Variable<ArrayType>>)
  //  &>;

  //  using DataType = typename ArrayType::type;

  std::size_t                      id;
  bool                             is_leaf;
  std::vector<Variable<ArrayType>> prev;
  function_signature               back_fn;
  ArrayType                        data;
  ArrayType                        grad;

  Variable(std::vector<std::size_t> in_shape, function_signature in_b_fn = nullptr,
           bool in_is_leaf = true)
  {
    data = ArrayType(in_shape);
    data.Reshape(in_shape);
    Setup(in_b_fn, in_is_leaf);
  }

  Variable(ArrayType in_data, function_signature in_b_fn = nullptr, bool in_is_leaf = true)
  {
    data = ArrayType(in_data);
    Setup(in_b_fn, in_is_leaf);
  }

  bool operator==(Variable const &other) const
  {
    return this->id == other.id;
  }

  void Backward()
  {
    assert(back_fn);

    for (std::size_t i = 0; i < grad.size(); ++i)
    {
      grad[i] = 1.0;
    }

    back_fn(*this);
  }

private:
  void Setup(function_signature in_b_fn, bool in_is_leaf)
  {
    // must either be a leaf node or have a back prop function
    assert(in_b_fn || in_is_leaf);

    // set a distinct id for each element in the graph
    this->id = variable_counter;
    variable_counter += 1;

    is_leaf = in_is_leaf;
    back_fn = in_b_fn;

    grad = ArrayType(data.shape());
    grad.data().SetAllZero();
  }
};
}  // namespace ml
}  // namespace fetch

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