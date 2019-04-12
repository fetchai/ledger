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

  // Convenince method to call without having to allocate output buffer
  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    ArrayType output(ComputeOutputShape(inputs));
    return Forward(inputs, output);
  }

  // Convenince method to call without having to allocate output buffer
  virtual ArrayType ForwardBatch(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    ArrayType output(ComputeOutputShape(inputs, true));
    return ForwardBatch(inputs, output);
  }

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output) = 0;
  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)                 = 0;
  virtual ArrayType ForwardBatch(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                                 ArrayType &output) = 0;
  virtual std::vector<ArrayType> BackwardBatch(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal) = 0;
  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) = 0;
  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs, bool batch)
  {
    if (batch)
    {
      auto      shape     = inputs.front().get().shape();
      SizeType  batchSize = shape.front();
      ArrayType slice     = inputs.front().get().Slice(0);
      shape               = ComputeOutputShape({slice});
      shape.insert(shape.begin(), batchSize);
      return shape;
    }
    return ComputeOutputShape(inputs);
  }

  void SetTraining(bool is_training)
  {
    is_training_ = is_training;
  }

protected:
  bool is_training_ = true;
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

  virtual ArrayType ForwardBatch(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                                 ArrayType &                                                 output)
  {
    return this->Forward(inputs, output);
  }

  virtual std::vector<ArrayType> BackwardBatch(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    return this->Backward(inputs, errorSignal);
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
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
  virtual ArrayType ForwardBatch(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                                 ArrayType &                                                 output)
  {
    assert(inputs.size() == 1);

    for (typename ArrayType::SizeType b(0); b < inputs.front().get().shape()[0]; ++b)
    {
      ArrayType input_slice  = inputs.front().get().Slice(b);
      ArrayType output_slice = output.Slice(b);
      this->Forward({input_slice}, output_slice);
    }
    return output;
  }

  virtual std::vector<ArrayType> BackwardBatch(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape()[0] == errorSignal.shape()[0]);
    std::vector<ArrayType> results;
    for (typename ArrayType::SizeType b(0); b < inputs.front().get().shape()[0]; ++b)
    {
      ArrayType inputSlice = inputs.front().get().Slice(b);
      ArrayType errorSlice = errorSignal.Slice(b);
      auto      ret        = this->Backward({inputSlice}, errorSlice);
      results.push_back(ret.front());
    }
    return {ConcatenateTensors(results)};
  }
};

}  // namespace ml
}  // namespace fetch
