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

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class CrossEntropyLayer
{
public:
  using ArrayType    = T;
  using Datatype     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  CrossEntropyLayer()          = default;
  virtual ~CrossEntropyLayer() = default;

  virtual typename ArrayType::Type Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->size() == inputs[1]->size());

    // we can't handle taking log(0), and the user should ensure this is never asked for
    for (std::size_t k = 0; k < inputs[0]->size(); ++k)
    {
      assert(inputs[0]->At(k) != 0);
    }

    // deep copy and take log of input
    ArrayType logx{inputs[0]->shape()};
    logx.Copy(*inputs[0]);
    fetch::math::Log(logx);

    // assuming 2D input[0],
    ArrayType plogx{logx.shape()};
    for (std::size_t j = 0; j < logx.size(); ++j)
    {
      if (inputs[1]->At(j) == 0)
      {
        plogx.Set(j, 0);
      }
      else if (inputs[1]->At(j) == 1)
      {
        plogx.Set(j, logx.At(j) * inputs[1]->At(j));
      }
      else
      {
        // should be a one hot
        assert(false);
      }
      //      else if (logx.At(j) == 0)
      //      {
      //        plogx.Set(j, 0);
      //      }
      //      else
      //      {
      //        plogx.Set(j, logx.At(j) * inputs[1]->At(j));
      //      }
    }

    ArrayType                cel      = fetch::math::Multiply(plogx, -1.0);
    typename ArrayType::Type n        = typename ArrayType::Type(cel.size());
    ArrayType                mean_cel = fetch::math::ReduceSum(cel, 1);

    ArrayType ret = fetch::math::Divide(mean_cel, n);
    assert(ret.size() == 1);
    return typename ArrayType::Type(ret[0]);
  }

  virtual ArrayPtrType Backward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->size() == inputs[1]->size());

    typename ArrayType::Type n_classes = static_cast<typename ArrayType::Type>(inputs[1]->size());

    ArrayPtrType ret = std::make_shared<ArrayType>(inputs[0]->shape());
    for (std::size_t i(0); i < inputs[0]->size(); ++i)
    {
      ret->At(i) = (inputs[0]->At(i) - inputs[1]->At(i)) / n_classes;
    }
    return ret;
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
