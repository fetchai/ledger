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
#include "ml/ops/weights.hpp"
#include <set>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Embeddings : public fetch::ml::ops::Weights<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Weights<T>::VecTensorType;

  Embeddings(SizeType dataPoints, SizeType dimensions)
  {
    ArrayType weights = ArrayType(std::vector<SizeType>({dataPoints, dimensions}));
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dataPoints, dimensions);
    this->SetData(weights);
  }

  Embeddings(ArrayType &weights)
  {
    this->SetData(weights);
  }

  virtual ~Embeddings() = default;

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    ASSERT(this->output_);
    ASSERT(inputs.size() == 1);
    ASSERT(
        (inputs.front().get().shape().size() == 1) ||
        ((inputs.front().get().shape().size() == 2) && (inputs.front().get().shape().at(1) == 1)));

    if (!this->embeddings_output_ ||
        this->embeddings_output_->shape()[0] != inputs.front().get().size() ||
        this->embeddings_output_->shape()[1] != this->output_->shape()[1])
    {
      this->embeddings_output_ = std::make_shared<ArrayType>(
          std::vector<SizeType>({inputs.front().get().size(), this->output_->shape()[1]}));
    }
    uint64_t j(0);
    for (DataType const &i : inputs.front().get())
    {
      auto tmp  = this->embeddings_output_->Slice(j);
      auto tmp2 = this->output_->Slice(typename ArrayType::SizeType(i));
      tmp.Assign(tmp2);
      j++;
    }
    output = *this->embeddings_output_;
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(
        (inputs.front().get().shape().size() == 1) ||
        ((inputs.front().get().shape().size() == 2) && (inputs.front().get().shape().at(1) == 1)));

    uint64_t j(0);
    for (DataType const &i : inputs.front().get())
    {
      updated_rows_.insert(typename ArrayType::SizeType(double(i)));
      this->gradient_accumulation_->Slice(typename ArrayType::SizeType(double(i)))
          .Assign(error_signal.Slice(j));
      j++;
    }
    return {ArrayType(error_signal.shape())};
  }

  virtual void Step(typename T::Type learning_rate)
  {
    ArrayType embedding_slice;

    for (auto const &r : updated_rows_)
    {
      // get the relevant slice from gradients and embeddings
      auto grad_slice = this->gradient_accumulation_->Slice(r);
      auto out_slice  = this->output_->Slice(r);

      embedding_slice = out_slice.Copy();

      // multiply accumulated gradients by learning rate, then subtract from current embeddings
      embedding_slice.InlineSubtract(grad_slice.Copy().InlineMultiply(learning_rate));

      // zero out gradients and assign new embeddings values
      grad_slice.Assign(ArrayType::Zeroes(embedding_slice.shape()));
      out_slice.Assign(embedding_slice);
    }
    updated_rows_.clear();
  }

private:
  ArrayPtrType                           embeddings_output_;
  std::set<typename ArrayType::SizeType> updated_rows_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
