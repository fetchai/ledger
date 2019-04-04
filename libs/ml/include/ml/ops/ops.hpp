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

/*
 * Abstract Ops interface
 */
template <class T>
class Ops
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs) = 0;
  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal) = 0;
  virtual ArrayType ForwardBatch(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) = 0;
  virtual std::vector<ArrayType> BackwardBatch(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal) = 0;
  virtual std::vector<SizeType> ComputeOutputSize(std::vector<std::reference_wrapper<ArrayType const>> const &inputs) = 0;

protected:
  ArrayPtrType output_;  // TODO(private, 736) -- Remove
};

/*
 * Abstract class for Ops that works element wise (relu, sigmoid, ...)
 */
template <class T>
class ElementWiseOps : public Ops<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  virtual ArrayType ForwardBatch(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    return this->Forward(inputs);
  }

  virtual std::vector<ArrayType> BackwardBatch(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    return this->Backward(inputs, errorSignal);
  }

  virtual std::vector<SizeType> ComputeOutputSize(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    return inputs.front().get().shape();
  }
};

/*
 * Abstract class for Ops that works with batch (convolution, softmax, ...)
 * (assuming a structure like [BATCH x OTHER_DIMS x ...])
 */
template <class T>
class BatchOps : public Ops<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

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

  virtual std::vector<ArrayType> BackwardBatch(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    return this->Backward(inputs, errorSignal);
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape()[0] == errorSignal.shape()[0]);
    std::vector<std::vector<ArrayType>> results;
    for (typename ArrayType::SizeType b(0); b < inputs.front().get().shape()[0]; ++b)
    {
      ArrayType inputSlice = inputs.front().get().Slice(b);
      ArrayType errorSlice = errorSignal.Slice(b);
      auto      ret        = this->Backward({inputSlice}, errorSlice);
      for (std::size_t i(0); i < ret.size(); ++i)
      {
        results[i].push_back(ret[i]);
      }
    }
    std::vector<ArrayType> concatenatedResults;
    for (auto const &tensorList : results)
    {
      concatenatedResults.push_back(ConcatenateTensors(tensorList));
    }
    return concatenatedResults;
  }
};

}  // namespace ml
}  // namespace fetch
