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
#include "math/tensor.hpp"
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
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = std::vector<std::reference_wrapper<ArrayType const>>;

  virtual ~Ops() = default;

  virtual void                   Forward(VecTensorType const &inputs, ArrayType &output)      = 0;
  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)                      = 0;
  virtual void                   ForwardBatch(VecTensorType const &inputs, ArrayType &output) = 0;
  virtual std::vector<ArrayType> BackwardBatch(VecTensorType const &inputs,
                                               ArrayType const &    error_signal)                 = 0;

  /*
   * ComputeOutputShape is usually expensive function and should be used only for initialization or
   * in ASSERT. On Forward you can use output.shape() and on Backward there is error_signal.shape()
   */
  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const = 0;

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
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  virtual void ForwardBatch(VecTensorType const &inputs, ArrayType &output)
  {
    this->Forward(inputs, output);
  }

  virtual std::vector<ArrayType> BackwardBatch(VecTensorType const &inputs,
                                               ArrayType const &    error_signal)
  {
    return this->Backward(inputs, error_signal);
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
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
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  // Individual ops are likely to overload this method for optimisation
  virtual void ForwardBatch(VecTensorType const &inputs, ArrayType &output)
  {

    // TODO (1048) - this implementation needs fixing

    assert(inputs.size() == 1);
    assert(output.shape()[output.shape().size() - 1] ==
           inputs.front().get().shape()[inputs.front().get().shape().size() - 1]);

    std::vector<ArrayType> results;
    for (typename ArrayType::SizeType b(0); b < inputs.front().get().shape()[0]; ++b)
    {
      ArrayType slice = inputs.front().get().Slice(b).Copy();
      this->Forward({slice}, output);
      results.push_back(output);
    }
    output = ArrayType::Stack(results);
  }

  virtual std::vector<ArrayType> BackwardBatch(VecTensorType const &inputs,
                                               ArrayType const &    error_signal)
  {
    return this->Backward(inputs, error_signal);

    // TODO (XXXXX) - why is there all this dead code?
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape()[0] == error_signal.shape()[0]);
    std::vector<std::vector<ArrayType>> results;
    for (typename ArrayType::SizeType b(0); b < inputs.front().get().shape()[0]; ++b)
    {
      auto input     = inputs.front().get().Slice(b).Copy();
      auto err_slice = error_signal.Slice(b).Copy();
      auto ret       = this->Backward({input}, err_slice);
      for (std::size_t i(0); i < ret.size(); ++i)
      {
        results[i].push_back(ret[i]);
      }
    }
    std::vector<ArrayType> concatenated_results;
    for (auto const &tensorList : results)
    {
      concatenated_results.push_back(ArrayType::Stack(tensorList));
    }
    return concatenated_results;
  }
};

}  // namespace ml
}  // namespace fetch
