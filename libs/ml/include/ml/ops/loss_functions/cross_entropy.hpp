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

#include <cassert>
#include <memory>
#include <vector>

#include "math/free_functions/fundamental_operators.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/free_functions/standard_functions/log.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class CrossEntropy
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  CrossEntropy()          = default;
  virtual ~CrossEntropy() = default;

  virtual typename ArrayType::Type Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->size() == inputs[1]->size());

    //    // we can't handle taking log(0), and the user should ensure this is never asked for
    //    for (std::uint64_t k = 0; k < inputs[0]->size(); ++k)
    //    {
    //      assert(inputs[0]->At(k) != 0);
    //    }

    std::uint64_t n_data    = inputs[0]->shape()[0];
    std::uint64_t n_classes = inputs[0]->shape()[1];

    // deep copy and take log of input
    ArrayType logx{inputs[0]->shape()};
    logx.Copy(*inputs[0]);
    fetch::math::Log(logx);

    // assuming 2D input[0],
    ArrayType plogx{logx.shape()};
    for (std::uint64_t i = 0; i < n_data; ++i)
    {
      for (std::uint64_t j = 0; j < n_classes; ++j)
      {
        if (inputs[1]->At(j) == DataType(0))
        {
          plogx.Set({i, j}, DataType(0));
        }
        else if (inputs[1]->At(j) == DataType(1))
        {
          plogx.Set({i, j}, logx.At(j));
        }
        else
        {
          std::cout << "input to cross entropy loss was not a one hot" << std::endl;
          assert(false);
        }
      }
    }
    ArrayType cel = fetch::math::Multiply(plogx, DataType(-1.0));

    // every true class label now has a negative log loss

    //    ArrayType                mean_cel = fetch::math::ReduceSum(cel, 1);
    typename ArrayType::Type ret = fetch::math::Divide(
        fetch::math::Sum(cel), static_cast<typename ArrayType::Type>(n_classes));
    //    ArrayType ret = fetch::math::Divide(mean_cel, n_classes);
    //    assert(ret.size() == inputs[0]->shape()[0]);
    return ret;
  }

  virtual ArrayPtrType Backward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->size() == inputs[1]->size());

    typename ArrayType::Type n_classes = static_cast<typename ArrayType::Type>(inputs[1]->size());

    ArrayPtrType ret = std::make_shared<ArrayType>(inputs[0]->shape());
    for (std::uint64_t i(0); i < inputs[0]->size(); ++i)
    {
      ret->At(i) = (inputs[0]->At(i) - inputs[1]->At(i)) / n_classes;
    }
    return ret;
  }

  static std::string Descriptor()
  {
    return "CrossEntropy";
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
