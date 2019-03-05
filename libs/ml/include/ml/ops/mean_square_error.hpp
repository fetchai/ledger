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
class MeanSquareErrorLayer
{
public:
  using ArrayType    = T;
  using Datatype     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  MeanSquareErrorLayer()          = default;
  virtual ~MeanSquareErrorLayer() = default;

  virtual typename ArrayType::Type Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->shape() == inputs[1]->shape());

    typename ArrayType::Type sum(0);
    for (std::uint64_t i(0); i < inputs[0]->size(); ++i)
    {
      sum += (inputs[0]->At(i) - inputs[1]->At(i)) * (inputs[0]->At(i) - inputs[1]->At(i));
    }
    sum /= Datatype(inputs[0]->shape()[0]);
    sum /= Datatype(2);  // TODO(private 343)
    return sum;
  }

  virtual ArrayPtrType Backward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->shape() == inputs[1]->shape());
    ArrayPtrType ret = std::make_shared<ArrayType>(inputs[0]->shape());
    for (std::uint64_t i(0); i < inputs[0]->size(); ++i)
    {
      ret->At(i) = (inputs[0]->At(i) - inputs[1]->At(i));
    }
    return ret;
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
