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

  Embeddings(SizeType dimensions, SizeType dataPoints)
  {
    ArrayType weights = ArrayType(std::vector<SizeType>({dimensions, dataPoints}));
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dimensions, dataPoints);
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
    ASSERT(inputs.front().get().shape().size() == 2);

    SizeType batch_size = inputs.front().get().shape(1);

    if (!this->embeddings_output_ ||
        this->embeddings_output_->shape().at(1) != inputs.front().get().shape().at(0) ||
        this->embeddings_output_->shape().at(0) != this->output_->shape().at(0))
    {
      this->embeddings_output_ = std::make_shared<ArrayType>(std::vector<SizeType>(
          {this->output_->shape().at(0), inputs.front().get().shape(0), batch_size}));
    }

    ArrayType transposed_input = inputs.front().get().Transpose();
    auto      e_it             = transposed_input.begin();
    for (SizeType i{0}; i < inputs.front().get().shape().at(0); i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {

        indices1.at(0)       = i;
        indices1.at(1)       = n;
        auto embedding_slice = this->embeddings_output_->View(indices1);
        indices2.at(0)       = static_cast<SizeType>(*e_it);
        auto output_slice    = this->output_->View(indices2);

        embedding_slice.Assign(output_slice);
        ++e_it;
      }
    }

    output = *this->embeddings_output_;
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(inputs.front().get().shape().size() == 2);

    SizeType batch_size = inputs.front().get().shape(1);

    ArrayType transposed_input = inputs.front().get().Transpose();
    auto      e_it             = transposed_input.begin();
    for (SizeType i{0}; i < inputs.front().get().shape().at(0); i++)
    {
      for (SizeType n{0}; n < batch_size; n++)
      {

        indices1.at(0)      = i;
        indices1.at(1)      = n;
        auto error_slice    = error_signal.View(indices1);
        indices2.at(0)      = static_cast<SizeType>(*e_it);
        auto gradient_slice = this->gradient_accumulation_->View(indices2);

        auto error_slice_it    = error_slice.cbegin();
        auto gradient_slice_it = gradient_slice.begin();
        while (error_slice_it.is_valid())
        {
          *gradient_slice_it += *error_slice_it;
          ++error_slice_it;
          ++gradient_slice_it;
        }
        ++e_it;
      }
    }

    return {ArrayType(error_signal.shape())};
  }

  virtual void Step(typename T::Type learning_rate)
  {
    ArrayType embedding_slice;

    for (auto const &r : updated_rows_)
    {
      // get the relevant slice from gradients and embeddings
      auto grad_slice = this->gradient_accumulation_->Slice(r, 1);
      auto out_slice  = this->output_->Slice(r, 1);

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
  std::vector<SizeType>                  indices1 = {0, 0};
  std::vector<SizeType>                  indices2 = {0};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
