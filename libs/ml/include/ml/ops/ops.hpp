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

#include "core/assert.hpp"
#include "math/tensor_operations.hpp"
#include <memory>
#include <vector>

namespace fetch {
namespace ml {

template <class T>
class Ops
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs) = 0;

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal) = 0;
  virtual ArrayType ForwardBatch(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) = 0;

protected:
  ArrayPtrType output_;  // TODO(private, 736) -- Remove
};

template <class T>
class ElementWiseOps : public Ops<T>
{
public:
  using ArrayType = T;

  virtual ArrayType ForwardBatch(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    return this->Forward(inputs);
  }
};

template <class T>
class BatchOps : public Ops<T>
{
public:
  using ArrayType = T;

  // Overload that method for optimisation purposes
  virtual ArrayType ForwardBatch(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    assert(inputs.size() == 1);
    std::vector<ArrayType> results;
    for (typename ArrayType::SizeType b(0); b < inputs.front().get().shape()[0]; ++b)
    {
      ArrayType slice = inputs.front().get().Slice(b);
      results.push_back(this->Forward({slice}));
    }
    return ConcatenateTensors(results);
  }
};

}  // namespace ml
}  // namespace fetch
